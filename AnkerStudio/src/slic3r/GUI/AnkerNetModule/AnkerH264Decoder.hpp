#ifndef ANKER_H264_DECODER_HPP
#define ANKER_H264_DECODER_HPP

#include <cstdint>
#include <cstddef>
#include <functional>

namespace AnkerNet {

// Hardware H.264 decoder (macOS VideoToolbox). Feed it the raw Annex-B elementary
// stream (as it arrives from the printer's P2P video channel); it parses NAL units,
// builds a format description from SPS/PPS, decodes on the GPU, and delivers each
// frame as tightly-packed RGB24 (width*height*3) via the frame callback -- exactly
// what AnkerVideo's wxImage(width,height,data) expects. Plain C++ interface (pimpl);
// implemented in AnkerH264Decoder.mm.
class AnkerH264Decoder
{
public:
    using FrameCb = std::function<void(const uint8_t* rgb24, int width, int height)>;

    AnkerH264Decoder();
    ~AnkerH264Decoder();

    AnkerH264Decoder(const AnkerH264Decoder&) = delete;
    AnkerH264Decoder& operator=(const AnkerH264Decoder&) = delete;

    void setFrameCallback(FrameCb cb);
    // Append bytes from the stream; decoded frames are delivered via the callback.
    void feed(const uint8_t* data, size_t len);

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace AnkerNet

#endif // ANKER_H264_DECODER_HPP
