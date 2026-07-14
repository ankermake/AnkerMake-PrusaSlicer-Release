#ifndef ANKER_MQTT_HPP
#define ANKER_MQTT_HPP

#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// Forward-declare OpenSSL types so this header stays free of <openssl/*.h>.
typedef struct ssl_st SSL;
typedef struct ssl_ctx_st SSL_CTX;

namespace AnkerNet {

// Minimal, self-contained MQTT 3.1.1 client over OpenSSL TLS. Just enough for the
// AnkerMake monitor use case: connect with username/password, subscribe (QoS 0),
// publish (QoS 0), keepalive pings, and a background receive loop that hands raw
// PUBLISH payloads (which are binary "MA"-framed blobs) to a callback. Payloads
// are byte-accurate std::strings (may contain NULs); lengths are always explicit.
class AnkerMqttClient
{
public:
    using MessageCb = std::function<void(const std::string& topic, const std::string& payload)>;

    AnkerMqttClient() = default;
    ~AnkerMqttClient();

    AnkerMqttClient(const AnkerMqttClient&) = delete;
    AnkerMqttClient& operator=(const AnkerMqttClient&) = delete;

    void setMessageCallback(MessageCb cb) { m_onMessage = std::move(cb); }

    // Blocking TLS connect + MQTT CONNECT handshake. Returns true on CONNACK ok.
    bool connect(const std::string& host, int port,
        const std::string& username, const std::string& password,
        const std::string& clientId);

    bool subscribe(const std::string& topic);
    bool publish(const std::string& topic, const std::string& payload);

    // Spawn the background receive/keepalive loop (call once, after connect()).
    void startLoop();
    void disconnect();
    bool isConnected() const { return m_connected.load(); }

private:
    bool tlsConnect(const std::string& host, int port);
    void tlsClose();
    bool writeAll(const uint8_t* data, size_t len);
    bool readN(uint8_t* data, size_t len);

    bool sendConnect(const std::string& username, const std::string& password,
        const std::string& clientId);
    bool sendPingReq();
    // Reads one MQTT control packet: its type nibble and remaining-length body.
    bool readPacket(uint8_t& type, std::vector<uint8_t>& body);
    void loop();

    SSL_CTX* m_ctx = nullptr;
    SSL* m_ssl = nullptr;
    int m_fd = -1;

    std::atomic<bool> m_connected{ false };
    std::atomic<bool> m_running{ false };
    std::thread m_thread;
    std::mutex m_writeMutex;

    MessageCb m_onMessage;
    uint16_t m_packetId = 1;
};

// AnkerMake "MA" message framing + AES-CBC payload codec (see protocol notes).
namespace mqttframe {

// Decrypts a received "MA" frame -> plaintext JSON. keyHex is the device secret_key
// (hex). Returns empty string on any framing/checksum/decrypt failure.
std::string decode(const std::string& frame, const std::string& keyHex);

// Wraps a JSON payload in a signed+encrypted "MA" frame for publishing. guid is a
// 36-char client UUID string. Returns empty string on failure.
std::string encode(const std::string& json, const std::string& keyHex, const std::string& guid);

} // namespace mqttframe

} // namespace AnkerNet

#endif // ANKER_MQTT_HPP
