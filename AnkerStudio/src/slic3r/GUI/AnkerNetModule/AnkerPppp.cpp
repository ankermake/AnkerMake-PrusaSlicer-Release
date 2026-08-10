#include "AnkerPppp.hpp"

#include <chrono>
#include <cstdio>
#include <cstring>

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <openssl/evp.h>

namespace AnkerNet {

namespace {

constexpr int PPPP_LAN_PORT = 32108;
constexpr int PPPP_WAN_PORT = 32100;

// Decode a p2p_conn init-string into its plaintext (a comma-separated list of relay
// server hosts). Port of pppp_decode_initstring (shuffle cipher).
std::string decodeInitString(const std::string& in)
{
    static const uint8_t shuffle[54] = {
        0x49, 0x59, 0x43, 0x3d, 0xb5, 0xbf, 0x6d, 0xa3, 0x47, 0x53,
        0x4f, 0x61, 0x65, 0xe3, 0x71, 0xe9, 0x67, 0x7f, 0x02, 0x03,
        0x0b, 0xad, 0xb3, 0x89, 0x2b, 0x2f, 0x35, 0xc1, 0x6b, 0x8b,
        0x95, 0x97, 0x11, 0xe5, 0xa7, 0x0d, 0xef, 0xf1, 0x05, 0x07,
        0x83, 0xfb, 0x9d, 0x3b, 0xc5, 0xc7, 0x13, 0x17, 0x1d, 0x1f,
        0x25, 0x29, 0xd3, 0xdf
    };
    size_t olen = in.size() >> 1;
    std::vector<uint8_t> out(olen, 0);
    for (size_t q = 0; q < olen; ++q) {
        uint8_t x = static_cast<uint8_t>(0x39 ^ shuffle[q % 0x36]);
        for (size_t p = 0; p <= q; ++p)
            x ^= out[p];
        int l = static_cast<uint8_t>(in[q * 2 + 1]) - 0x41;
        int h = static_cast<uint8_t>(in[q * 2 + 0]) - 0x41;
        out[q] = static_cast<uint8_t>(x ^ static_cast<uint8_t>(l + (h << 4)));
    }
    return std::string(out.begin(), out.end());
}

// PPPP "simple" stream cipher (megajank simple_encrypt/decrypt), used to obfuscate
// the REPORT_SESSION_READY (0xf9) body. seed = "SSD@cs2-network.".
const uint8_t PPPP_SIMPLE_SHUFFLE[256] = {
    0x7C,0x9C,0xE8,0x4A,0x13,0xDE,0xDC,0xB2,0x2F,0x21,0x23,0xE4,0x30,0x7B,0x3D,0x8C,
    0xBC,0x0B,0x27,0x0C,0x3C,0xF7,0x9A,0xE7,0x08,0x71,0x96,0x00,0x97,0x85,0xEF,0xC1,
    0x1F,0xC4,0xDB,0xA1,0xC2,0xEB,0xD9,0x01,0xFA,0xBA,0x3B,0x05,0xB8,0x15,0x87,0x83,
    0x28,0x72,0xD1,0x8B,0x5A,0xD6,0xDA,0x93,0x58,0xFE,0xAA,0xCC,0x6E,0x1B,0xF0,0xA3,
    0x88,0xAB,0x43,0xC0,0x0D,0xB5,0x45,0x38,0x4F,0x50,0x22,0x66,0x20,0x7F,0x07,0x5B,
    0x14,0x98,0x1D,0x9B,0xA7,0x2A,0xB9,0xA8,0xCB,0xF1,0xFC,0x49,0x47,0x06,0x3E,0xB1,
    0x0E,0x04,0x3A,0x94,0x5E,0xEE,0x54,0x11,0x34,0xDD,0x4D,0xF9,0xEC,0xC7,0xC9,0xE3,
    0x78,0x1A,0x6F,0x70,0x6B,0xA4,0xBD,0xA9,0x5D,0xD5,0xF8,0xE5,0xBB,0x26,0xAF,0x42,
    0x37,0xD8,0xE1,0x02,0x0A,0xAE,0x5F,0x1C,0xC5,0x73,0x09,0x4E,0x69,0x24,0x90,0x6D,
    0x12,0xB3,0x19,0xAD,0x74,0x8A,0x29,0x40,0xF5,0x2D,0xBE,0xA5,0x59,0xE0,0xF4,0x79,
    0xD2,0x4B,0xCE,0x89,0x82,0x48,0x84,0x25,0xC6,0x91,0x2B,0xA2,0xFB,0x8F,0xE9,0xA6,
    0xB0,0x9E,0x3F,0x65,0xF6,0x03,0x31,0x2E,0xAC,0x0F,0x95,0x2C,0x5C,0xED,0x39,0xB7,
    0x33,0x6C,0x56,0x7E,0xB4,0xA0,0xFD,0x7A,0x81,0x53,0x51,0x86,0x8D,0x9F,0x77,0xFF,
    0x6A,0x80,0xDF,0xE2,0xBF,0x10,0xD7,0x75,0x64,0x57,0x76,0xF3,0x55,0xCD,0xD0,0xC8,
    0x18,0xE6,0x36,0x41,0x62,0xCF,0x99,0xF2,0x32,0x4C,0x67,0x60,0x61,0x92,0xCA,0xD3,
    0xEA,0x63,0x7D,0x16,0xB6,0x8E,0xD4,0x68,0x35,0xC3,0x52,0x9D,0x46,0x44,0x1E,0x17,
};

// simple_hash of the seed -> 4 ints (reversed). Values may be large/negative; kept as
// int to mirror the Python reference exactly before the % 256 in the lookup.
void simpleHash(const char* seed, size_t n, int h[4])
{
    int t[4] = { 0, 0, 0, 0 };
    for (size_t i = 0; i < n; ++i) {
        uint8_t c = static_cast<uint8_t>(seed[i]);
        t[0] ^= c; t[1] += c / 3; t[2] -= c; t[3] += c;
    }
    h[0] = t[3]; h[1] = t[2]; h[2] = t[1]; h[3] = t[0];
}

uint8_t simpleLookup(const int h[4], int b)
{
    long idx = static_cast<long>(h[b & 3]) + b;    // may be negative
    long m = idx % 256; if (m < 0) m += 256;        // Python-style modulo
    return PPPP_SIMPLE_SHUFFLE[m];
}

// Encrypt in place (out[i] = in[i] ^ lookup(h, out[i-1])).
std::vector<uint8_t> simpleEncrypt(const std::vector<uint8_t>& in)
{
    static const char SEED[] = "SSD@cs2-network.";
    int h[4]; simpleHash(SEED, sizeof(SEED) - 1, h);
    std::vector<uint8_t> out(in.size());
    if (in.empty()) return out;
    out[0] = in[0] ^ simpleLookup(h, 0);
    for (size_t i = 1; i < in.size(); ++i)
        out[i] = in[i] ^ simpleLookup(h, out[i - 1]);
    return out;
}

// Wire Host.addr is the IPv4 octets byte-REVERSED (little-endian of network order).
// Convert 4 wire bytes -> s_addr (network order).
uint32_t hostAddrToSaddr(const uint8_t* wire)
{
    uint8_t b[4] = { wire[3], wire[2], wire[1], wire[0] };
    uint32_t s; std::memcpy(&s, b, 4); return s;
}
// Append a 16-byte Host with s_addr (network order) reversed into wire order.
void putHostLE(std::vector<uint8_t>& v, uint32_t saddr, uint16_t port)
{
    v.push_back(0);            // pad0
    v.push_back(2);            // afam = AF_INET
    v.push_back(static_cast<uint8_t>(port & 0xff));        // port LE
    v.push_back(static_cast<uint8_t>((port >> 8) & 0xff));
    const uint8_t* a = reinterpret_cast<const uint8_t*>(&saddr);
    v.push_back(a[3]); v.push_back(a[2]); v.push_back(a[1]); v.push_back(a[0]); // addr reversed
    v.insert(v.end(), 8, 0);   // pad1
}

// Hex string -> raw bytes (used for the dsk_key).
std::string unhex(const std::string& h)
{
    if (h.size() % 2)
        return "";
    auto val = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };
    std::string out;
    out.reserve(h.size() / 2);
    for (size_t i = 0; i < h.size(); i += 2) {
        int hi = val(h[i]), lo = val(h[i + 1]);
        if (hi < 0 || lo < 0)
            return "";
        out.push_back(static_cast<char>((hi << 4) | lo));
    }
    return out;
}

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
    T_P2P_REQ = 0x20, T_P2P_REQ_ACK = 0x21, T_P2P_REQ_DSK = 0x26,
    T_PUNCH_TO = 0x40,
    T_P2P_RDY = 0x42, T_P2P_RDY_ACK = 0x43,
    T_DRW = 0xd0, T_DRW_ACK = 0xd1,
    T_ALIVE = 0xe0, T_ALIVE_ACK = 0xe1,
    T_CLOSE = 0xf0,
    T_SESSION_READY = 0xf9, // REPORT_SESSION_READY (ICE candidate exchange)
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
    // Before we've locked onto a peer (broadcast discovery / relay rendezvous), only
    // the DUID-bearing discovery replies and STUN/rendezvous packets are actionable.
    if (!m_peerLocked && type != T_P2P_RDY && type != T_PUNCH_PKT
        && type != T_P2P_REQ_ACK && type != T_PUNCH_TO && type != T_HELLO_ACK)
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
    case T_HELLO_ACK:
        // STUN reply from the relay: payload is a 16-byte Host = our public/WAN address.
        // Keep it verbatim for the REPORT_SESSION_READY addr_wan candidate.
        if (!m_gotWanHost && p.size() >= 16) {
            m_wanHost.assign(p.begin(), p.begin() + 16);
            m_gotWanHost = true;
            uint16_t wport = static_cast<uint16_t>(p[2] | (p[3] << 8));
            struct in_addr ia; ia.s_addr = hostAddrToSaddr(&p[4]);
            std::fprintf(stderr, "[AnkerPppp] STUN: our WAN %s:%u\n", inet_ntoa(ia), wport);
        }
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
    case T_P2P_REQ_ACK: {
        // Relay accepted the rendezvous. Do NOT lock the peer here -- we still need to
        // collect candidates/WAN host from every relay and later accept the device's
        // reply from an ephemeral port during the punch.
        m_p2pReqAcked = true;
        break;
    }
    case T_PUNCH_TO: {
        // Relay handed us the device's public address. Aim our hole-punch at it, but
        // stay UNLOCKED: the device's reply often arrives from a different NAT port,
        // and we must accept it (dispatch locks on the matching P2P_RDY/PUNCH_PKT).
        if (p.size() >= 8) {
            uint16_t dport = static_cast<uint16_t>(p[2] | (p[3] << 8));
            uint32_t daddr = hostAddrToSaddr(&p[4]); // wire addr is byte-reversed
            struct in_addr ia;
            ia.s_addr = daddr;
            std::fprintf(stderr, "[AnkerPppp] PUNCH_TO candidate %s:%u\n", inet_ntoa(ia), dport);
            // Add as a candidate (dedup, skip empty); we punch all of them.
            if (daddr != 0 && dport != 0) {
                bool have = false;
                for (const auto& t : m_punchTargets)
                    if (t.first == daddr && t.second == dport) { have = true; break; }
                if (!have)
                    m_punchTargets.emplace_back(daddr, dport);
            }
            m_punching = true; // loop now hammers PUNCH_PKT at all candidates
        }
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
            if (!m_connected) {
                struct in_addr ia;
                ia.s_addr = srcAddr;
                std::fprintf(stderr, "[AnkerPppp] rx type=0x%02x len=%u from %s:%u\n",
                    type, len, inet_ntoa(ia), srcPort);
            }
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

bool AnkerPpppClient::sendPktTo(uint32_t addr, uint16_t port, uint8_t type,
    const std::vector<uint8_t>& payload)
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
    dst.sin_port = htons(port);
    dst.sin_addr.s_addr = addr;
    ssize_t n = ::sendto(m_fd, pkt.data(), pkt.size(), 0,
        reinterpret_cast<struct sockaddr*>(&dst), sizeof(dst));
    return n == static_cast<ssize_t>(pkt.size());
}

bool AnkerPpppClient::connectRelay(const std::string& duidStr, const std::string& dskHex,
    const std::string& p2pConn, int timeoutSec)
{
    m_duid = packDuid(duidStr);
    if (m_duid.empty() || p2pConn.empty())
        return false;

    // dsk_key from get_dsk_keys is a 20-char RAW key (not hex). Use its bytes directly.
    std::string dsk = dskHex;
    dsk.resize(20, '\0'); // Dsk.key is 20 bytes
    std::fprintf(stderr, "[AnkerPppp] relay dsk rawlen=%zu\n", dskHex.size());

    // p2p_conn init-string -> comma-separated relay hosts.
    std::string hosts = decodeInitString(p2pConn);
    std::vector<std::string> hostList;
    {
        std::string cur;
        for (char c : hosts) {
            if (c == ',') { if (!cur.empty()) hostList.push_back(cur); cur.clear(); }
            else cur.push_back(c);
        }
        if (!cur.empty()) hostList.push_back(cur);
    }
    if (hostList.empty())
        return false;
    std::fprintf(stderr, "[AnkerPppp] relay hosts: %s\n", hosts.c_str());

    m_fd = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (m_fd < 0)
        return false;
    m_connected = false;
    m_closed = false;
    m_peerLocked = false;
    m_punching = false;
    m_punchTargets.clear();
    m_p2pReqAcked = false;
    m_gotWanHost = false;
    m_wanHost.clear();

    // P2P_REQ_DSK payload: duid(20) + host(16) + nat_type(1) + version(3) + dsk(20+4).
    std::vector<uint8_t> req;
    req.insert(req.end(), m_duid.begin(), m_duid.end());
    req.push_back(0);            // host.pad0
    req.push_back(2);            // host.afam = AF_INET
    putU16le(req, 0);           // host.port
    req.insert(req.end(), 4, 0); // host.addr 0.0.0.0
    req.insert(req.end(), 8, 0); // host.pad1
    req.push_back(0);            // nat_type
    req.push_back(1); req.push_back(0); req.push_back(0); // version
    req.insert(req.end(), dsk.begin(), dsk.begin() + 20);
    req.insert(req.end(), 4, 0); // dsk.pad

    // Resolve every relay host once; we broadcast to all of them for redundancy.
    std::vector<uint32_t> relayAddrs;
    for (const auto& h : hostList) {
        struct addrinfo hints; std::memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET; hints.ai_socktype = SOCK_DGRAM;
        struct addrinfo* res = nullptr;
        if (getaddrinfo(h.c_str(), nullptr, &hints, &res) == 0 && res) {
            relayAddrs.push_back(reinterpret_cast<struct sockaddr_in*>(res->ai_addr)->sin_addr.s_addr);
            freeaddrinfo(res);
        }
    }
    if (relayAddrs.empty())
        return false;

    long long deadline = nowMs() + static_cast<long long>(timeoutSec) * 1000;
    long long nextSend = 0;

    // Phase 1: STUN + rendezvous. To every relay, send HELLO (learn our public/WAN Host
    // from HELLO_ACK) and P2P_REQ_DSK (rendezvous -> P2P_REQ_ACK + the device's PUNCH_TO
    // candidate addresses). Stay unlocked so replies from all relays are accepted. Cap
    // this phase so the punch always gets the bulk of the window even if STUN is slow.
    long long phase1Deadline = nowMs() + 6000;
    if (phase1Deadline > deadline) phase1Deadline = deadline;
    m_peerLocked = false;
    while ((!m_gotWanHost || !m_p2pReqAcked || m_punchTargets.empty())
        && nowMs() < phase1Deadline && !m_closed) {
        if (nowMs() >= nextSend) {
            for (uint32_t ra : relayAddrs) {
                sendPktTo(ra, PPPP_WAN_PORT, T_HELLO, {});
                sendPktTo(ra, PPPP_WAN_PORT, T_P2P_REQ_DSK, req);
            }
            nextSend = nowMs() + 500;
        }
        pump(100);
    }
    if (!m_p2pReqAcked || m_punchTargets.empty()) {
        std::fprintf(stderr, "[AnkerPppp] remote: rendezvous incomplete (wan=%d req=%d cands=%zu)\n",
            m_gotWanHost ? 1 : 0, m_p2pReqAcked ? 1 : 0, m_punchTargets.size());
        return false;
    }

    // Our local LAN address/port for the addr_local candidate (best effort; only
    // addr_wan matters to a remote peer, but the app always includes addr_local).
    uint32_t localAddr = 0; uint16_t localPort = 0;
    {
        struct sockaddr_in sn; socklen_t sl = sizeof(sn);
        if (getsockname(m_fd, reinterpret_cast<struct sockaddr*>(&sn), &sl) == 0) {
            localPort = ntohs(sn.sin_port); localAddr = sn.sin_addr.s_addr;
        }
        if (localAddr == 0) { // discover routable local IP via a temp connected socket
            int t = ::socket(AF_INET, SOCK_DGRAM, 0);
            if (t >= 0) {
                struct sockaddr_in r; std::memset(&r, 0, sizeof(r));
                r.sin_family = AF_INET; r.sin_port = htons(PPPP_WAN_PORT);
                r.sin_addr.s_addr = relayAddrs[0];
                if (::connect(t, reinterpret_cast<struct sockaddr*>(&r), sizeof(r)) == 0) {
                    struct sockaddr_in ln; socklen_t ll = sizeof(ln);
                    if (getsockname(t, reinterpret_cast<struct sockaddr*>(&ln), &ll) == 0)
                        localAddr = ln.sin_addr.s_addr;
                }
                ::close(t);
            }
        }
    }

    // Build REPORT_SESSION_READY (0xf9): duid(20) + middle(16) + addr_local + addr_wan +
    // addr_relay, encrypted with the simple cipher. This tells the device our addresses
    // so it can punch BACK to us. addr_wan is the verbatim STUN Host from HELLO_ACK.
    static const uint8_t MIDDLE[16] = {
        0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01,
        0x01, 0x91, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x00 };
    std::vector<uint8_t> sr;
    sr.insert(sr.end(), m_duid.begin(), m_duid.end());
    sr.insert(sr.end(), MIDDLE, MIDDLE + 16);
    putHostLE(sr, localAddr, localPort);                      // addr_local
    if (m_wanHost.size() == 16)
        sr.insert(sr.end(), m_wanHost.begin(), m_wanHost.end()); // addr_wan (verbatim)
    else
        putHostLE(sr, 0, 0);
    putHostLE(sr, 0, 0);                                      // addr_relay (unused)
    std::vector<uint8_t> srEnc = simpleEncrypt(sr);

    // Phase 2: punch. Repeatedly announce our candidates (REPORT_SESSION_READY, relayed
    // to the device) and hammer PUNCH_PKT + P2P_RDY at the device's candidate addresses.
    // Once the device knows our address it punches back; its P2P_RDY (from an ephemeral
    // port) locks the peer and sets m_connected. The official app often fails the first
    // round and succeeds on a retry, so we keep at it for the whole window.
    std::fprintf(stderr, "[AnkerPppp] remote: punching %zu candidate(s)\n", m_punchTargets.size());
    m_peerLocked = false;
    nextSend = 0;
    while (!m_connected && nowMs() < deadline && !m_closed) {
        if (nowMs() >= nextSend) {
            for (uint32_t ra : relayAddrs)
                sendPktTo(ra, PPPP_WAN_PORT, T_SESSION_READY, srEnc);
            for (const auto& t : m_punchTargets) {
                sendPktTo(t.first, t.second, T_PUNCH_PKT, m_duid);
                sendPktTo(t.first, t.second, T_P2P_RDY, m_duid);
            }
            nextSend = nowMs() + 300;
        }
        pump(100);
    }

    std::fprintf(stderr, "[AnkerPppp] remote connect %s\n", m_connected ? "succeeded" : "timed out");
    return m_connected;
}

bool AnkerPpppClient::connect(const std::string& duidStr, const std::string& ip,
    const std::string& dskHex, const std::string& p2pConn, int timeoutSec)
{
    // LAN first (fast on the same subnet).
    if (connectLan(duidStr, ip, 8))
        return true;
    close();
    // Remote fallback via Anker's relay servers.
    if (!dskHex.empty() && !p2pConn.empty())
        return connectRelay(duidStr, dskHex, p2pConn, timeoutSec);
    std::fprintf(stderr, "[AnkerPppp] no relay creds (dsk/p2p_conn) for remote fallback\n");
    return false;
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
        if (!waitDrained(0, 10000)) {
            std::fprintf(stderr, "[AnkerPppp] P2P_SEND_FILE announce not acked (connection unstable?)\n");
            return false;
        }
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
