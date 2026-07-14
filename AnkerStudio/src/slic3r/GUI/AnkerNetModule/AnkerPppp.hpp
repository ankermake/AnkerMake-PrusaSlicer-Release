#ifndef ANKER_PPPP_HPP
#define ANKER_PPPP_HPP

#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace AnkerNet {

// Minimal PPPP (P2P over UDP) client for the AnkerMake M5 LAN file transfer that
// starts a print. Synchronous "pump" design (no background thread): higher-level
// operations repeatedly call pump() until their completion condition. Covers only
// the LAN path (printer reachable at its ip_addr on UDP 32108), which needs no
// dsk_key. See the pppp-protocol memory for the full wire format.
class AnkerPpppClient
{
public:
    // (bytesSent, bytesTotal) during file upload.
    using ProgressCb = std::function<void(size_t, size_t)>;

    AnkerPpppClient() = default;
    ~AnkerPpppClient();

    // Connect over LAN. duidStr is the device p2p_did ("prefix-serial-check").
    // Returns true once the session reaches Connected within timeoutSec.
    bool connectLan(const std::string& duidStr, const std::string& ip, int timeoutSec = 15);

    // Upload fileData and trigger a print. Metadata identity fields are lenient
    // (ankerctl uses placeholders). Returns true when the printer replies OK to
    // the final data frame. progress may be null.
    bool uploadFile(const std::string& fileName, const std::string& fileData,
        const std::string& userName, const std::string& userId,
        const std::string& machineId, const ProgressCb& progress);

    // Stage-1 camera proof: request the printer's live stream and write the raw
    // H.264 elementary stream to outPath until maxBytes captured (or timeout),
    // then stop the stream. Playable with ffplay/vlc. Returns bytes captured > 0.
    bool captureVideo(const std::string& outPath, size_t maxBytes, int timeoutSec);

    // Live-stream primitives (Stage 2): request the stream, pull one H.264 chunk at
    // a time (caller loops on its own thread and feeds a decoder), then stop.
    void startLive();
    bool recvVideoChunk(std::vector<uint8_t>& out, int timeoutMs);
    void stopLive();

    void close();
    bool isConnected() const { return m_connected; }

private:
    // One reliable ordered byte stream (DRW/DRW_ACK) identified by channel index.
    struct Channel {
        // tx: index -> (payload, next-retransmit deadline in ms-since-epoch)
        struct TxPkt { std::vector<uint8_t> data; long long deadline; };
        std::map<uint16_t, TxPkt> tx;   // unacked, keyed by packet index
        uint16_t txCtr = 0;             // next tx index to assign

        std::map<uint16_t, std::vector<uint8_t>> rxBuf; // out-of-order hold
        uint16_t rxCtr = 0;             // next expected rx index
        std::vector<uint8_t> rxBytes;   // in-order delivered bytes (consumed by readN)
    };

    // Low-level packet send: F1 | type | len(BE) | payload.
    bool sendPkt(uint8_t type, const std::vector<uint8_t>& payload);
    // Queue `data` for reliable delivery on `chan`, split into <=1024B DRW packets.
    void channelSend(uint8_t chan, const std::vector<uint8_t>& data);
    // Process one inbound datagram + run retransmits. Returns false on socket error.
    bool pump(int timeoutMs);
    void dispatch(uint8_t type, const std::vector<uint8_t>& payload, uint32_t srcAddr, uint16_t srcPort);
    void retransmit();
    // Pump until channel has no unacked tx packets (or timeout).
    bool waitDrained(uint8_t chan, int timeoutMs);
    // Pump until `n` in-order bytes are available on chan, then pop them into out.
    bool readN(uint8_t chan, size_t n, std::vector<uint8_t>& out, int timeoutMs);
    // send one AABB frame + await the printer's REPLY frame; true if reply==OK.
    bool aabbRequest(uint8_t frametype, const std::vector<uint8_t>& data, uint32_t pos,
        int timeoutMs);
    // Frame `data` as an XZYH message with command `cmd` and queue it on `chan`.
    void sendXzyh(uint16_t cmd, const std::vector<uint8_t>& data, uint8_t chan);
    // Read one XZYH frame's payload from `chan` into out (used for video chunks).
    bool recvXzyh(uint8_t chan, std::vector<uint8_t>& out, int timeoutMs);

    // True once we've locked onto the target printer's address (directed connect,
    // or a broadcast LAN_SEARCH response whose DUID matched). Until then, discovery
    // packets from other printers are inspected but not adopted as the peer.
    bool duidMatches(const std::vector<uint8_t>& payload) const;

    int m_fd = -1;
    // sockaddr_in of the printer (updated from each recvfrom); stored opaquely.
    uint32_t m_peerAddr = 0; // network-order IPv4
    uint16_t m_peerPort = 0; // host order
    bool m_peerLocked = false;
    std::vector<uint8_t> m_duid; // packed 20-byte Duid

    bool m_connected = false;
    bool m_closed = false;
    Channel m_chan[8];
};

} // namespace AnkerNet

#endif // ANKER_PPPP_HPP
