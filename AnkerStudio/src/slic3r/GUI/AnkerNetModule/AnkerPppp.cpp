#include "AnkerPppp.hpp"

#include <chrono>
#include <cstdio>
#include <cstring>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <openssl/evp.h>

namespace AnkerNet {

namespace {

constexpr int PPPP_LAN_PORT = 32108;

// Lowercase hex MD5 of data into out[33] (32 chars + NUL). Uses OpenSSL EVP.
void md5_hex(const std::string& data, char out[33])
{
    unsigned char digest[16] = { 0 };
    unsigned int dlen = 0;
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (ctx) {
        if (EVP_DigestInit_ex(ctx, EVP_md5(), nullptr) == 1 &&
            EVP_DigestUpdate(ctx, data.data(), data.size()) == 1) {
            EVP_DigestFinal_ex(ctx, digest, &dlen);
        }
        EVP_MD_CTX_free(ctx);
    }
    static const char* hx = "0123456789abcdef";
    for (int i = 0; i < 16; ++i) {
        out[i * 2] = hx[(digest[i] >> 4) & 0xf];
        out[i * 2 + 1] = hx[digest[i] & 0xf];
    }
    out[32] = '\0';
}

// PPPP message types.
enum PType : uint8_t {
    T_HELLO = 0x00, T_HELLO_ACK = 0x01,
    T_DEV_LGN_CRC = 0x12,
    T_LAN_SEARCH = 0x30,
    T_PUNCH_PKT = 0x41,
    T_P2P_RDY = 0x42, T_P2P_RDY_ACK = 0x43,
    T_DRW = 0xd0, T_DRW_ACK = 0xd1,
    T_ALIVE = 0xe0, T_ALIVE_ACK = 0xe1,
    T_CLOSE = 0xf0,
};

// FileTransfer frametypes.
enum FType : uint8_t { FT_BEGIN = 0x00, FT_DATA = 0x01, FT_END = 0x02, FT_REPLY = 0x80 };

constexpr uint16_t P2P_SEND_FILE = 0x3a98;
constexpr uint16_t P2P_JSON_CMD = 0x06a4;
constexpr int SUB_START_LIVE = 0x03e8; // 1000
constexpr int SUB_CLOSE_LIVE = 0x03e9; // 1001

long long nowMs()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

uint16_t crc16_xmodem(const uint8_t* data, size_t len)
{
    uint16_t crc = 0x0000;
    for (size_t i = 0; i < len; ++i) {
        crc ^= static_cast<uint16_t>(data[i]) << 8;
        for (int b = 0; b < 8; ++b)
            crc = (crc & 0x8000) ? static_cast<uint16_t>((crc << 1) ^ 0x1021)
                                 : static_cast<uint16_t>(crc << 1);
    }
    return crc;
}

void putU16be(std::vector<uint8_t>& v, uint16_t x)
{
    v.push_back(static_cast<uint8_t>(x >> 8));
    v.push_back(static_cast<uint8_t>(x & 0xff));
}
void putU16le(std::vector<uint8_t>& v, uint16_t x)
{
    v.push_back(static_cast<uint8_t>(x & 0xff));
    v.push_back(static_cast<uint8_t>(x >> 8));
}
void putU32le(std::vector<uint8_t>& v, uint32_t x)
{
    v.push_back(static_cast<uint8_t>(x & 0xff));
    v.push_back(static_cast<uint8_t>((x >> 8) & 0xff));
    v.push_back(static_cast<uint8_t>((x >> 16) & 0xff));
    v.push_back(static_cast<uint8_t>((x >> 24) & 0xff));
}

// Pack a Duid "prefix-serial-check" -> 20 bytes: prefix(8) + serial(u32 BE) + check(6) + pad(2).
std::vector<uint8_t> packDuid(const std::string& s)
{
    std::vector<uint8_t> out;
    auto d1 = s.find('-');
    auto d2 = s.rfind('-');
    if (d1 == std::string::npos || d2 == d1)
        return out;
    std::string prefix = s.substr(0, d1);
    std::string serialStr = s.substr(d1 + 1, d2 - d1 - 1);
    std::string check = s.substr(d2 + 1);
    uint32_t serial = static_cast<uint32_t>(strtoul(serialStr.c_str(), nullptr, 10));

    out.resize(20, 0);
    for (size_t i = 0; i < prefix.size() && i < 8; ++i)
        out[i] = static_cast<uint8_t>(prefix[i]);
    out[8] = static_cast<uint8_t>((serial >> 24) & 0xff);
    out[9] = static_cast<uint8_t>((serial >> 16) & 0xff);
    out[10] = static_cast<uint8_t>((serial >> 8) & 0xff);
    out[11] = static_cast<uint8_t>(serial & 0xff);
    for (size_t i = 0; i < check.size() && i < 6; ++i)
        out[12 + i] = static_cast<uint8_t>(check[i]);
    // out[18..19] pad = 0
    return out;
}

} // namespace

AnkerPpppClient::~AnkerPpppClient()
{
    close();
}

bool AnkerPpppClient::sendPkt(uint8_t type, const std::vector<uint8_t>& payload)
{
    std::vector<uint8_t> pkt;
    pkt.reserve(4 + payload.size());
    pkt.push_back(0xF1);
    pkt.push_back(type);
    putU16be(pkt, static_cast<uint16_t>(payload.size()));
    pkt.insert(pkt.end(), payload.begin(), payload.end());

    struct sockaddr_in dst;
    std::memset(&dst, 0, sizeof(dst));
    dst.sin_family = AF_INET;
    dst.sin_port = htons(m_peerPort);
    dst.sin_addr.s_addr = m_peerAddr;
    ssize_t n = ::sendto(m_fd, pkt.data(), pkt.size(), 0,
        reinterpret_cast<struct sockaddr*>(&dst), sizeof(dst));
    return n == static_cast<ssize_t>(pkt.size());
}

// Build a Host struct (16 bytes) for the current peer address.
static std::vector<uint8_t> packPeerHost(uint32_t peerAddr, uint16_t peerPort)
{
    std::vector<uint8_t> h;
    h.push_back(0);              // pad0
    h.push_back(2);              // afam = AF_INET
    putU16le(h, peerPort);       // port (LE)
    const uint8_t* a = reinterpret_cast<const uint8_t*>(&peerAddr); // network order = octets
    h.insert(h.end(), a, a + 4); // addr
    h.insert(h.end(), 8, 0);     // pad1
    return h;
}

void AnkerPpppClient::channelSend(uint8_t chan, const std::vector<uint8_t>& data)
{
    Channel& ch = m_chan[chan];
    size_t off = 0;
    while (off < data.size()) {
        size_t n = std::min<size_t>(1024, data.size() - off);
        std::vector<uint8_t> body;               // DRW payload
        body.push_back(0xD1);                     // signature
        body.push_back(chan);
        putU16be(body, ch.txCtr);
        body.insert(body.end(), data.begin() + off, data.begin() + off + n);

        ch.tx[ch.txCtr] = { body, nowMs() };      // deadline now -> sent immediately in retransmit()
        ch.txCtr++;
        off += n;
    }
    retransmit();
}

void AnkerPpppClient::retransmit()
{
    long long t = nowMs();
    for (int c = 0; c < 8; ++c) {
        for (auto& kv : m_chan[c].tx) {
            if (kv.second.deadline <= t) {
                sendPkt(T_DRW, kv.second.data);
                kv.second.deadline = t + 500; // 0.5s
            }
        }
    }
}

// Match a printer's DUID (from a P2P_RDY/PUNCH payload) against our target. Compare
// the prefix(8)+serial(4) portion, which is the reliable identity; the trailing
// check bytes can be reported differently by the device.
bool AnkerPpppClient::duidMatches(const std::vector<uint8_t>& payload) const
{
    if (payload.size() < 12 || m_duid.size() < 12)
        return false;
    return std::memcmp(payload.data(), m_duid.data(), 12) == 0;
}

void AnkerPpppClient::dispatch(uint8_t type, const std::vector<uint8_t>& p,
    uint32_t srcAddr, uint16_t srcPort)
{
    // Before we've locked onto a peer (broadcast discovery), only the DUID-bearing
    // discovery replies are actionable; ignore everything else.
    if (!m_peerLocked && type != T_P2P_RDY && type != T_PUNCH_PKT)
        return;

    switch (type) {
    case T_P2P_RDY: {
        if (!m_peerLocked) {
            if (!duidMatches(p))
                return; // a different printer answered the broadcast
            m_peerAddr = srcAddr;
            m_peerPort = srcPort;
            m_peerLocked = true;
            std::fprintf(stderr, "[AnkerPppp] locked peer via P2P_RDY\n");
        }
        std::vector<uint8_t> ack = m_duid;
        auto host = packPeerHost(m_peerAddr, m_peerPort);
        ack.insert(ack.end(), host.begin(), host.end());
        ack.insert(ack.end(), 8, 0); // pad
        sendPkt(T_P2P_RDY_ACK, ack);
        m_connected = true;
        break;
    }
    case T_PUNCH_PKT:
        if (!m_connected) {
            if (!m_peerLocked) {
                if (!duidMatches(p))
                    return;
                m_peerAddr = srcAddr;
                m_peerPort = srcPort;
                m_peerLocked = true;
                std::fprintf(stderr, "[AnkerPppp] locked peer via PUNCH\n");
            }
            sendPkt(T_CLOSE, {});
            std::vector<uint8_t> rdy = m_duid;
            sendPkt(T_P2P_RDY, rdy);
        }
        break;
    case T_HELLO:
        sendPkt(T_HELLO_ACK, packPeerHost(m_peerAddr, m_peerPort));
        break;
    case T_ALIVE:
        sendPkt(T_ALIVE_ACK, {});
        break;
    case T_CLOSE:
        m_closed = true;
        m_connected = false;
        break;
    case T_DRW_ACK: {
        // signature(1) chan(1) count(u16 BE) acks[u16 BE]...
        if (p.size() < 4)
            break;
        uint8_t chan = p[1];
        uint16_t count = static_cast<uint16_t>((p[2] << 8) | p[3]);
        for (uint16_t i = 0; i < count && 4 + i * 2 + 1 < p.size(); ++i) {
            uint16_t idx = static_cast<uint16_t>((p[4 + i * 2] << 8) | p[4 + i * 2 + 1]);
            m_chan[chan].tx.erase(idx);
        }
        break;
    }
    case T_DRW: {
        // signature(1) chan(1) index(u16 BE) data...
        if (p.size() < 4)
            break;
        uint8_t chan = p[1];
        uint16_t index = static_cast<uint16_t>((p[2] << 8) | p[3]);
        std::vector<uint8_t> data(p.begin() + 4, p.end());

        // Ack every received DRW.
        std::vector<uint8_t> ack;
        ack.push_back(0xD1);
        ack.push_back(chan);
        putU16be(ack, 1);
        putU16be(ack, index);
        sendPkt(T_DRW_ACK, ack);

        Channel& ch = m_chan[chan];
        uint16_t diff = static_cast<uint16_t>(index - ch.rxCtr);
        if (diff == 0) {
            ch.rxBytes.insert(ch.rxBytes.end(), data.begin(), data.end());
            ch.rxCtr++;
            auto it = ch.rxBuf.find(ch.rxCtr);
            while (it != ch.rxBuf.end()) {
                ch.rxBytes.insert(ch.rxBytes.end(), it->second.begin(), it->second.end());
                ch.rxBuf.erase(it);
                ch.rxCtr++;
                it = ch.rxBuf.find(ch.rxCtr);
            }
        } else if (diff < 0x8000) {
            ch.rxBuf[index] = std::move(data); // future packet
        } // else: already-seen duplicate, ignore
        break;
    }
    case T_DEV_LGN_CRC:
        // LAN path: best-effort, no crypto. If a printer demands a valid CRC ack this
        // would need megajank crypto_curse; observed LAN flows connect via P2P_RDY.
        break;
    default:
        break;
    }
}

bool AnkerPpppClient::pump(int timeoutMs)
{
    struct timeval tv;
    tv.tv_sec = timeoutMs / 1000;
    tv.tv_usec = (timeoutMs % 1000) * 1000;
    setsockopt(m_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    uint8_t buf[4096];
    struct sockaddr_in src;
    socklen_t slen = sizeof(src);
    ssize_t n = ::recvfrom(m_fd, buf, sizeof(buf), 0,
        reinterpret_cast<struct sockaddr*>(&src), &slen);
    if (n >= 4 && buf[0] == 0xF1) {
        uint32_t srcAddr = src.sin_addr.s_addr;
        uint16_t srcPort = ntohs(src.sin_port);
        // Once locked, ignore packets from other hosts; otherwise track the peer's
        // (possibly ephemeral) response port.
        bool accept = !m_peerLocked || srcAddr == m_peerAddr;
        if (accept) {
            if (m_peerLocked) {
                m_peerAddr = srcAddr;
                m_peerPort = srcPort;
            }
            uint8_t type = buf[1];
            uint16_t len = static_cast<uint16_t>((buf[2] << 8) | buf[3]);
            size_t avail = static_cast<size_t>(n) - 4;
            if (len > avail)
                len = static_cast<uint16_t>(avail);
            std::vector<uint8_t> payload(buf + 4, buf + 4 + len);
            dispatch(type, payload, srcAddr, srcPort);
        }
    }
    retransmit();
    return n > 0;
}

bool AnkerPpppClient::waitDrained(uint8_t chan, int timeoutMs)
{
    long long deadline = nowMs() + timeoutMs;
    while (nowMs() < deadline) {
        if (m_chan[chan].tx.empty())
            return true;
        if (m_closed)
            return false;
        pump(50);
    }
    return m_chan[chan].tx.empty();
}

bool AnkerPpppClient::readN(uint8_t chan, size_t n, std::vector<uint8_t>& out, int timeoutMs)
{
    long long deadline = nowMs() + timeoutMs;
    Channel& ch = m_chan[chan];
    while (ch.rxBytes.size() < n) {
        if (m_closed || nowMs() >= deadline)
            return false;
        pump(50);
    }
    out.assign(ch.rxBytes.begin(), ch.rxBytes.begin() + n);
    ch.rxBytes.erase(ch.rxBytes.begin(), ch.rxBytes.begin() + n);
    return true;
}

bool AnkerPpppClient::aabbRequest(uint8_t frametype, const std::vector<uint8_t>& data,
    uint32_t pos, int timeoutMs)
{
    // Build AABB frame: AA BB | frametype | sn | pos(LE) | len(LE) | data | crc16(LE)
    std::vector<uint8_t> frame;
    frame.push_back(0xAA);
    frame.push_back(0xBB);
    frame.push_back(frametype);
    frame.push_back(0); // sn
    putU32le(frame, pos);
    putU32le(frame, static_cast<uint32_t>(data.size()));
    frame.insert(frame.end(), data.begin(), data.end());
    // CRC over header-after-magic (bytes [2:]) + data.
    uint16_t crc = crc16_xmodem(frame.data() + 2, frame.size() - 2);
    putU16le(frame, crc);

    channelSend(1, frame);
    if (!waitDrained(1, timeoutMs))
        return false;

    // Read the printer's AABB REPLY frame (12-byte header + len bytes + 2 crc).
    std::vector<uint8_t> hdr;
    if (!readN(1, 12, hdr, timeoutMs))
        return false;
    if (hdr[0] != 0xAA || hdr[1] != 0xBB)
        return false;
    uint32_t rlen = static_cast<uint32_t>(hdr[8]) | (hdr[9] << 8) | (hdr[10] << 16) | (hdr[11] << 24);
    std::vector<uint8_t> rest;
    if (!readN(1, rlen + 2, rest, timeoutMs))
        return false;
    if (rlen < 1)
        return false;
    uint8_t reply = rest[0];
    if (reply != 0x00)
        std::fprintf(stderr, "[AnkerPppp] aabb reply frametype=0x%02x reply=0x%02x\n", hdr[2], reply);
    return reply == 0x00;
}

bool AnkerPpppClient::connectLan(const std::string& duidStr, const std::string& ip, int timeoutSec)
{
    m_duid = packDuid(duidStr);
    if (m_duid.empty()) {
        std::fprintf(stderr, "[AnkerPppp] bad duid '%s'\n", duidStr.c_str());
        return false;
    }

    m_fd = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (m_fd < 0)
        return false;

    m_connected = false;
    m_closed = false;

    // Always enable broadcast so we can fall back to DUID discovery. If a directed
    // IP is given, try it first for a few seconds (the cloud's ip_addr is often empty
    // or stale), then switch to broadcast discovery for the remainder.
    int one = 1;
    setsockopt(m_fd, SOL_SOCKET, SO_BROADCAST, &one, sizeof(one));

    bool directed = !ip.empty() && inet_addr(ip.c_str()) != INADDR_NONE;
    long long switchToBroadcast = directed ? (nowMs() + 6000) : 0;
    if (directed) {
        m_peerAddr = inet_addr(ip.c_str());
        m_peerPort = PPPP_LAN_PORT;
        m_peerLocked = true;
        std::fprintf(stderr, "[AnkerPppp] trying directed connect to %s\n", ip.c_str());
    } else {
        m_peerAddr = inet_addr("255.255.255.255");
        m_peerPort = PPPP_LAN_PORT;
        m_peerLocked = false;
        std::fprintf(stderr, "[AnkerPppp] discovering printer via LAN broadcast\n");
    }

    long long deadline = nowMs() + timeoutSec * 1000;
    long long nextSearch = 0;
    while (!m_connected && nowMs() < deadline && !m_closed) {
        if (directed && switchToBroadcast && nowMs() >= switchToBroadcast) {
            directed = false;
            m_peerLocked = false;
            m_peerAddr = inet_addr("255.255.255.255");
            m_peerPort = PPPP_LAN_PORT;
            std::fprintf(stderr, "[AnkerPppp] directed connect stalled; switching to broadcast\n");
        }
        if (nowMs() >= nextSearch) {
            sendPkt(T_LAN_SEARCH, {}); // to broadcast addr while discovering, else to peer
            nextSearch = nowMs() + 500;
        }
        pump(100);
    }

    if (m_connected)
        std::fprintf(stderr, "[AnkerPppp] connected (peer locked)\n");
    else
        std::fprintf(stderr, "[AnkerPppp] connect timed out\n");
    return m_connected;
}

bool AnkerPpppClient::uploadFile(const std::string& fileName, const std::string& fileData,
    const std::string& userName, const std::string& userId,
    const std::string& machineId, const ProgressCb& progress)
{
    if (!m_connected)
        return false;

    // 1) XZYH on channel 0 announcing a file transfer.
    {
        // 16-char id (matches ankerctl's uuid4[:16]); content not otherwise used.
        std::string id16 = (machineId + "0000000000000000").substr(0, 16);
        sendXzyh(P2P_SEND_FILE, std::vector<uint8_t>(id16.begin(), id16.end()), 0);
        if (!waitDrained(0, 10000))
            return false;
    }

    // 2) BEGIN with the metadata CSV (+ trailing NUL).
    {
        char md5hex[33];
        md5_hex(fileData, md5hex);

        std::string csv = "0," + fileName + "," + std::to_string(fileData.size()) + "," +
            std::string(md5hex) + "," + userName + "," + userId + "," + machineId;
        std::vector<uint8_t> meta(csv.begin(), csv.end());
        meta.push_back(0x00);
        if (!aabbRequest(FT_BEGIN, meta, 0, 15000)) {
            std::fprintf(stderr, "[AnkerPppp] BEGIN rejected\n");
            return false;
        }
    }

    // 3) DATA in 32KB chunks.
    const size_t blocksize = 1024 * 32;
    size_t pos = 0;
    while (pos < fileData.size()) {
        size_t n = std::min(blocksize, fileData.size() - pos);
        std::vector<uint8_t> chunk(fileData.begin() + pos, fileData.begin() + pos + n);
        if (!aabbRequest(FT_DATA, chunk, static_cast<uint32_t>(pos), 20000)) {
            std::fprintf(stderr, "[AnkerPppp] DATA rejected at pos %zu\n", pos);
            return false;
        }
        pos += n;
        if (progress)
            progress(pos, fileData.size());
    }

    // 4) END (empty payload) finalizes the transfer and starts the print. Without
    // this the printer rejects the job ("transfer failed") despite acking each frame.
    if (!aabbRequest(FT_END, {}, 0, 20000)) {
        std::fprintf(stderr, "[AnkerPppp] END rejected\n");
        return false;
    }

    std::fprintf(stderr, "[AnkerPppp] upload complete + END (%zu bytes)\n", fileData.size());
    return true;
}

void AnkerPpppClient::sendXzyh(uint16_t cmd, const std::vector<uint8_t>& data, uint8_t chan)
{
    std::vector<uint8_t> body;
    const char* magic = "XZYH";
    body.insert(body.end(), magic, magic + 4);
    putU16le(body, cmd);
    putU32le(body, static_cast<uint32_t>(data.size()));
    body.insert(body.end(), 6, 0); // unk0,unk1,chan,sign_code,unk3,dev_type
    body.insert(body.end(), data.begin(), data.end());
    channelSend(chan, body);
}

bool AnkerPpppClient::recvXzyh(uint8_t chan, std::vector<uint8_t>& out, int timeoutMs)
{
    std::vector<uint8_t> hdr;
    if (!readN(chan, 16, hdr, timeoutMs))
        return false;
    if (!(hdr[0] == 'X' && hdr[1] == 'Z' && hdr[2] == 'Y' && hdr[3] == 'H'))
        return false;
    uint32_t len = static_cast<uint32_t>(hdr[6]) | (hdr[7] << 8) | (hdr[8] << 16) | (hdr[9] << 24);
    return readN(chan, len, out, timeoutMs);
}

bool AnkerPpppClient::captureVideo(const std::string& outPath, size_t maxBytes, int timeoutSec)
{
    if (!m_connected)
        return false;

    // Request the live stream (JSON command over XZYH on channel 0).
    std::string startJson = "{\"commandType\":" + std::to_string(SUB_START_LIVE) +
        ",\"data\":{\"encryptkey\":\"x\",\"accountId\":\"y\"}}";
    sendXzyh(P2P_JSON_CMD, std::vector<uint8_t>(startJson.begin(), startJson.end()), 0);
    waitDrained(0, 5000);

    FILE* f = std::fopen(outPath.c_str(), "wb");
    if (!f)
        return false;

    size_t total = 0;
    long long deadline = nowMs() + static_cast<long long>(timeoutSec) * 1000;
    while (total < maxBytes && nowMs() < deadline && !m_closed) {
        std::vector<uint8_t> frame;
        if (recvXzyh(1, frame, 3000)) {
            std::fwrite(frame.data(), 1, frame.size(), f);
            total += frame.size();
        }
    }
    std::fclose(f);

    // Stop the stream.
    std::string stopJson = "{\"commandType\":" + std::to_string(SUB_CLOSE_LIVE) + "}";
    sendXzyh(P2P_JSON_CMD, std::vector<uint8_t>(stopJson.begin(), stopJson.end()), 0);
    waitDrained(0, 3000);

    std::fprintf(stderr, "[AnkerPppp] captured %zu bytes of H.264 to %s\n", total, outPath.c_str());
    return total > 0;
}

void AnkerPpppClient::startLive()
{
    std::string startJson = "{\"commandType\":" + std::to_string(SUB_START_LIVE) +
        ",\"data\":{\"encryptkey\":\"x\",\"accountId\":\"y\"}}";
    sendXzyh(P2P_JSON_CMD, std::vector<uint8_t>(startJson.begin(), startJson.end()), 0);
    waitDrained(0, 5000);
}

bool AnkerPpppClient::recvVideoChunk(std::vector<uint8_t>& out, int timeoutMs)
{
    return recvXzyh(1, out, timeoutMs);
}

void AnkerPpppClient::stopLive()
{
    if (m_fd < 0)
        return;
    std::string stopJson = "{\"commandType\":" + std::to_string(SUB_CLOSE_LIVE) + "}";
    sendXzyh(P2P_JSON_CMD, std::vector<uint8_t>(stopJson.begin(), stopJson.end()), 0);
    waitDrained(0, 2000);
}

void AnkerPpppClient::close()
{
    if (m_fd >= 0) {
        if (m_connected)
            sendPkt(T_CLOSE, {});
        ::close(m_fd);
        m_fd = -1;
    }
    m_connected = false;
}

} // namespace AnkerNet
