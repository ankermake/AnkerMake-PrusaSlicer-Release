#include "AnkerMqtt.hpp"

#include <chrono>
#include <cstdio>
#include <cstring>

#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/evp.h>

namespace AnkerNet {

namespace {

// --- small helpers ---------------------------------------------------------

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

// AES-CBC with PKCS7 padding (OpenSSL default). key length selects 128/256.
std::string aes_cbc(bool encrypt, const std::string& key, const std::string& iv, const std::string& in)
{
    const EVP_CIPHER* cipher = (key.size() == 32) ? EVP_aes_256_cbc() : EVP_aes_128_cbc();
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx)
        return "";

    std::string out;
    out.resize(in.size() + 32);
    int len = 0, total = 0;
    bool ok =
        EVP_CipherInit_ex(ctx, cipher, nullptr,
            reinterpret_cast<const unsigned char*>(key.data()),
            reinterpret_cast<const unsigned char*>(iv.data()), encrypt ? 1 : 0) == 1 &&
        EVP_CipherUpdate(ctx, reinterpret_cast<unsigned char*>(&out[0]), &len,
            reinterpret_cast<const unsigned char*>(in.data()), static_cast<int>(in.size())) == 1;
    if (ok) {
        total = len;
        ok = EVP_CipherFinal_ex(ctx, reinterpret_cast<unsigned char*>(&out[0]) + total, &len) == 1;
        total += len;
    }
    EVP_CIPHER_CTX_free(ctx);
    if (!ok)
        return "";
    out.resize(total);
    return out;
}

uint8_t xor_all(const std::string& s)
{
    uint8_t x = 0;
    for (unsigned char c : s)
        x ^= c;
    return x;
}

// MQTT remaining-length varint encode.
void encodeRemainingLength(std::vector<uint8_t>& out, size_t len)
{
    do {
        uint8_t b = len % 128;
        len /= 128;
        if (len > 0)
            b |= 0x80;
        out.push_back(b);
    } while (len > 0);
}

void appendString(std::vector<uint8_t>& out, const std::string& s)
{
    out.push_back(static_cast<uint8_t>((s.size() >> 8) & 0xff));
    out.push_back(static_cast<uint8_t>(s.size() & 0xff));
    out.insert(out.end(), s.begin(), s.end());
}

} // namespace

// ---------------------------------------------------------------------------
// mqttframe: "MA" framing + AES payload codec
// ---------------------------------------------------------------------------

namespace mqttframe {

std::string decode(const std::string& frame, const std::string& keyHex)
{
    if (frame.size() < 65)
        return "";
    if (xor_all(frame) != 0)
        return ""; // checksum mismatch (last byte should make total XOR 0)

    std::string p = frame.substr(0, frame.size() - 1); // strip checksum byte
    if (p.size() < 64)
        return "";
    if (static_cast<uint8_t>(p[0]) != 'M' || static_cast<uint8_t>(p[1]) != 'A')
        return "";
    if (static_cast<uint8_t>(p[6]) != 2)
        return "";

    std::string key = unhex(keyHex);
    if (key.empty())
        return "";
    return aes_cbc(false, key, "3DPrintAnkerMake", p.substr(64));
}

std::string encode(const std::string& json, const std::string& keyHex, const std::string& guid)
{
    std::string key = unhex(keyHex);
    if (key.empty())
        return "";
    std::string enc = aes_cbc(true, key, "3DPrintAnkerMake", json);
    if (enc.empty())
        return "";

    const uint16_t size = static_cast<uint16_t>(64 + enc.size() + 1);
    std::string h;
    h.reserve(64);
    h.push_back('M');
    h.push_back('A');
    h.push_back(static_cast<char>(size & 0xff));
    h.push_back(static_cast<char>((size >> 8) & 0xff));
    h.push_back(5);
    h.push_back(1);
    h.push_back(2);
    h.push_back(5);
    h.push_back(static_cast<char>('F'));
    h.push_back(static_cast<char>(0xc0)); // packet_type = Single
    h.push_back(0);
    h.push_back(0); // packet_num u16le
    h.push_back(0);
    h.push_back(0);
    h.push_back(0);
    h.push_back(0); // time u32le
    std::string g = guid;
    g.resize(37, '\0');
    h.append(g);       // device_guid (37)
    h.append(11, '\0'); // padding (11)  -> header is now 64 bytes

    std::string body = h + enc;
    body.push_back(static_cast<char>(xor_all(body)));
    return body;
}

} // namespace mqttframe

// ---------------------------------------------------------------------------
// AnkerMqttClient
// ---------------------------------------------------------------------------

AnkerMqttClient::~AnkerMqttClient()
{
    disconnect();
}

bool AnkerMqttClient::tlsConnect(const std::string& host, int port)
{
    struct addrinfo hints;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo* res = nullptr;
    if (getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &res) != 0 || !res) {
        std::fprintf(stderr, "[AnkerMqtt] getaddrinfo failed for %s\n", host.c_str());
        return false;
    }

    int fd = -1;
    for (struct addrinfo* ai = res; ai; ai = ai->ai_next) {
        fd = ::socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (fd < 0)
            continue;
        if (::connect(fd, ai->ai_addr, ai->ai_addrlen) == 0)
            break;
        ::close(fd);
        fd = -1;
    }
    freeaddrinfo(res);
    if (fd < 0) {
        std::fprintf(stderr, "[AnkerMqtt] tcp connect failed\n");
        return false;
    }

    static std::once_flag sslInit;
    std::call_once(sslInit, []() {
        SSL_library_init();
        SSL_load_error_strings();
        OpenSSL_add_all_algorithms();
    });

    m_ctx = SSL_CTX_new(TLS_client_method());
    if (!m_ctx) {
        ::close(fd);
        return false;
    }
    // Anker's broker; we don't ship their CA, so don't verify the chain (matches
    // the community client's `insecure` option). Traffic is still TLS-encrypted.
    SSL_CTX_set_verify(m_ctx, SSL_VERIFY_NONE, nullptr);

    m_ssl = SSL_new(m_ctx);
    if (!m_ssl) {
        ::close(fd);
        SSL_CTX_free(m_ctx);
        m_ctx = nullptr;
        return false;
    }
    SSL_set_fd(m_ssl, fd);
    SSL_set_tlsext_host_name(m_ssl, host.c_str()); // SNI
    if (SSL_connect(m_ssl) != 1) {
        std::fprintf(stderr, "[AnkerMqtt] TLS handshake failed\n");
        SSL_free(m_ssl);
        m_ssl = nullptr;
        ::close(fd);
        SSL_CTX_free(m_ctx);
        m_ctx = nullptr;
        return false;
    }

    // 1s read timeout so the receive loop can send keepalive pings / exit cleanly.
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    m_fd = fd;
    return true;
}

void AnkerMqttClient::tlsClose()
{
    if (m_ssl) {
        SSL_shutdown(m_ssl);
        SSL_free(m_ssl);
        m_ssl = nullptr;
    }
    if (m_fd >= 0) {
        ::close(m_fd);
        m_fd = -1;
    }
    if (m_ctx) {
        SSL_CTX_free(m_ctx);
        m_ctx = nullptr;
    }
}

bool AnkerMqttClient::writeAll(const uint8_t* data, size_t len)
{
    std::lock_guard<std::mutex> lock(m_writeMutex);
    size_t sent = 0;
    while (sent < len) {
        int n = SSL_write(m_ssl, data + sent, static_cast<int>(len - sent));
        if (n <= 0) {
            int err = SSL_get_error(m_ssl, n);
            if (err == SSL_ERROR_WANT_WRITE || err == SSL_ERROR_WANT_READ)
                continue;
            return false;
        }
        sent += static_cast<size_t>(n);
    }
    return true;
}

bool AnkerMqttClient::readN(uint8_t* data, size_t len)
{
    size_t got = 0;
    while (got < len) {
        int n = SSL_read(m_ssl, data + got, static_cast<int>(len - got));
        if (n > 0) {
            got += static_cast<size_t>(n);
            continue;
        }
        int err = SSL_get_error(m_ssl, n);
        if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
            if (!m_running.load() && !m_connected.load())
                return false;
            continue; // read timeout mid-packet; keep waiting
        }
        return false;
    }
    return true;
}

bool AnkerMqttClient::sendConnect(const std::string& username, const std::string& password,
    const std::string& clientId)
{
    std::vector<uint8_t> vh;
    appendString(vh, "MQTT");        // protocol name
    vh.push_back(0x04);              // protocol level 4 (3.1.1)
    vh.push_back(0xC2);              // flags: username + password + clean session
    vh.push_back(0x00);
    vh.push_back(0x3C);              // keepalive = 60s
    appendString(vh, clientId);
    appendString(vh, username);
    appendString(vh, password);

    std::vector<uint8_t> pkt;
    pkt.push_back(0x10); // CONNECT
    encodeRemainingLength(pkt, vh.size());
    pkt.insert(pkt.end(), vh.begin(), vh.end());
    return writeAll(pkt.data(), pkt.size());
}

bool AnkerMqttClient::sendPingReq()
{
    const uint8_t pkt[2] = { 0xC0, 0x00 };
    return writeAll(pkt, sizeof(pkt));
}

// Reads a full control packet. Returns true with type+body filled; type=0 means
// "timeout, nothing read" (not an error). Returns false on connection error.
bool AnkerMqttClient::readPacket(uint8_t& type, std::vector<uint8_t>& body)
{
    type = 0;
    body.clear();

    uint8_t first = 0;
    int n = SSL_read(m_ssl, &first, 1);
    if (n <= 0) {
        int err = SSL_get_error(m_ssl, n);
        if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)
            return true; // timeout, type stays 0
        return false;
    }

    // remaining length varint
    size_t remaining = 0, multiplier = 1;
    for (int i = 0; i < 4; ++i) {
        uint8_t b = 0;
        if (!readN(&b, 1))
            return false;
        remaining += (b & 0x7f) * multiplier;
        if (!(b & 0x80))
            break;
        multiplier *= 128;
    }

    body.resize(remaining);
    if (remaining > 0 && !readN(body.data(), remaining))
        return false;

    type = first;
    return true;
}

bool AnkerMqttClient::connect(const std::string& host, int port,
    const std::string& username, const std::string& password,
    const std::string& clientId)
{
    if (!tlsConnect(host, port))
        return false;

    if (!sendConnect(username, password, clientId)) {
        tlsClose();
        return false;
    }

    // Await CONNACK.
    uint8_t type = 0;
    std::vector<uint8_t> body;
    for (int tries = 0; tries < 10; ++tries) {
        if (!readPacket(type, body)) {
            tlsClose();
            return false;
        }
        if (type == 0)
            continue; // timeout, retry
        break;
    }
    if ((type & 0xF0) != 0x20 || body.size() < 2 || body[1] != 0x00) {
        std::fprintf(stderr, "[AnkerMqtt] CONNACK rejected (type=0x%02x rc=%d)\n",
            type, body.size() >= 2 ? body[1] : -1);
        tlsClose();
        return false;
    }

    m_connected.store(true);
    std::fprintf(stderr, "[AnkerMqtt] connected to %s:%d\n", host.c_str(), port);
    return true;
}

bool AnkerMqttClient::subscribe(const std::string& topic)
{
    std::vector<uint8_t> vh;
    uint16_t pid = m_packetId++;
    vh.push_back(static_cast<uint8_t>((pid >> 8) & 0xff));
    vh.push_back(static_cast<uint8_t>(pid & 0xff));
    appendString(vh, topic);
    vh.push_back(0x00); // QoS 0

    std::vector<uint8_t> pkt;
    pkt.push_back(0x82); // SUBSCRIBE
    encodeRemainingLength(pkt, vh.size());
    pkt.insert(pkt.end(), vh.begin(), vh.end());
    return writeAll(pkt.data(), pkt.size());
}

bool AnkerMqttClient::publish(const std::string& topic, const std::string& payload)
{
    std::vector<uint8_t> vh;
    appendString(vh, topic); // QoS 0 -> no packet id
    vh.insert(vh.end(), payload.begin(), payload.end());

    std::vector<uint8_t> pkt;
    pkt.push_back(0x30); // PUBLISH, QoS 0
    encodeRemainingLength(pkt, vh.size());
    pkt.insert(pkt.end(), vh.begin(), vh.end());
    return writeAll(pkt.data(), pkt.size());
}

void AnkerMqttClient::loop()
{
    auto lastPing = std::chrono::steady_clock::now();

    while (m_running.load()) {
        uint8_t type = 0;
        std::vector<uint8_t> body;
        if (!readPacket(type, body)) {
            std::fprintf(stderr, "[AnkerMqtt] receive loop: connection lost\n");
            m_connected.store(false);
            break;
        }

        if (type != 0 && (type & 0xF0) == 0x30) {
            // PUBLISH: topic (len-prefixed), then payload (QoS 0 -> no packet id).
            if (body.size() >= 2) {
                size_t tlen = (static_cast<size_t>(body[0]) << 8) | body[1];
                if (2 + tlen <= body.size()) {
                    std::string topic(reinterpret_cast<char*>(body.data() + 2), tlen);
                    std::string payload(reinterpret_cast<char*>(body.data() + 2 + tlen),
                        body.size() - 2 - tlen);
                    if (m_onMessage)
                        m_onMessage(topic, payload);
                }
            }
        }

        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - lastPing).count() >= 30) {
            sendPingReq();
            lastPing = now;
        }
    }
}

void AnkerMqttClient::startLoop()
{
    if (m_running.exchange(true))
        return;
    m_thread = std::thread(&AnkerMqttClient::loop, this);
}

void AnkerMqttClient::disconnect()
{
    m_running.store(false);
    m_connected.store(false);
    if (m_thread.joinable())
        m_thread.join();
    tlsClose();
}

} // namespace AnkerNet
