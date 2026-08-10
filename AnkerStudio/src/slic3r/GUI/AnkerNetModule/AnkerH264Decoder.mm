#include "AnkerH264Decoder.hpp"

#include <cstdio>
#include <mutex>
#include <vector>

#import <CoreFoundation/CoreFoundation.h>
#import <CoreMedia/CoreMedia.h>
#import <CoreVideo/CoreVideo.h>
#import <VideoToolbox/VideoToolbox.h>

namespace AnkerNet {

struct AnkerH264Decoder::Impl {
    FrameCb cb;
    std::vector<uint8_t> buf;   // accumulated Annex-B stream
    std::vector<uint8_t> sps;
    std::vector<uint8_t> pps;
    std::vector<uint8_t> rgb;   // persistent RGB24 output (referenced by the UI)
    std::mutex rgbMutex;

    CMVideoFormatDescriptionRef fmt = nullptr;
    VTDecompressionSessionRef session = nullptr;

    ~Impl() { teardown(); }

    void teardown()
    {
        if (session) {
            VTDecompressionSessionInvalidate(session);
            CFRelease(session);
            session = nullptr;
        }
        if (fmt) {
            CFRelease(fmt);
            fmt = nullptr;
        }
    }

    void emitFrame(CVImageBufferRef img)
    {
        size_t w = CVPixelBufferGetWidth(img);
        size_t h = CVPixelBufferGetHeight(img);
        CVPixelBufferLockBaseAddress(img, kCVPixelBufferLock_ReadOnly);
        const uint8_t* base = static_cast<const uint8_t*>(CVPixelBufferGetBaseAddress(img));
        size_t stride = CVPixelBufferGetBytesPerRow(img);

        FrameCb localCb;
        {
            std::lock_guard<std::mutex> lock(rgbMutex);
            rgb.resize(w * h * 3);
            // Source is BGRA; pack to RGB24.
            for (size_t y = 0; y < h; ++y) {
                const uint8_t* srow = base + y * stride;
                uint8_t* drow = rgb.data() + y * w * 3;
                for (size_t x = 0; x < w; ++x) {
                    drow[x * 3 + 0] = srow[x * 4 + 2]; // R
                    drow[x * 3 + 1] = srow[x * 4 + 1]; // G
                    drow[x * 3 + 2] = srow[x * 4 + 0]; // B
                }
            }
            localCb = cb;
        }
        CVPixelBufferUnlockBaseAddress(img, kCVPixelBufferLock_ReadOnly);

        static int frameCount = 0;
        if (frameCount++ % 60 == 0)
            std::fprintf(stderr, "[H264] delivered frame %d (%zux%zu)\n", frameCount, w, h);

        if (localCb)
            localCb(rgb.data(), static_cast<int>(w), static_cast<int>(h));
    }

    void createSession()
    {
        teardown();

        const uint8_t* params[2] = { sps.data(), pps.data() };
        const size_t sizes[2] = { sps.size(), pps.size() };
        OSStatus st = CMVideoFormatDescriptionCreateFromH264ParameterSets(
            kCFAllocatorDefault, 2, params, sizes, 4, &fmt);
        if (st != noErr) {
            std::fprintf(stderr, "[H264] format desc failed: %d\n", static_cast<int>(st));
            fmt = nullptr;
            return;
        }

        CFMutableDictionaryRef attrs = CFDictionaryCreateMutable(
            kCFAllocatorDefault, 1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
        SInt32 fmtType = kCVPixelFormatType_32BGRA;
        CFNumberRef num = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &fmtType);
        CFDictionarySetValue(attrs, kCVPixelBufferPixelFormatTypeKey, num);
        CFRelease(num);

        VTDecompressionOutputCallbackRecord cbRec;
        cbRec.decompressionOutputCallback = &Impl::outputCallback;
        cbRec.decompressionOutputRefCon = this;

        st = VTDecompressionSessionCreate(kCFAllocatorDefault, fmt, nullptr, attrs, &cbRec, &session);
        CFRelease(attrs);
        if (st != noErr) {
            std::fprintf(stderr, "[H264] session create failed: %d\n", static_cast<int>(st));
            session = nullptr;
        } else {
            std::fprintf(stderr, "[H264] decode session ready\n");
        }
    }

    static void outputCallback(void* refcon, void* /*srcFrameRefCon*/, OSStatus status,
        VTDecodeInfoFlags /*flags*/, CVImageBufferRef imageBuffer, CMTime /*pts*/, CMTime /*dur*/)
    {
        if (status != noErr || !imageBuffer)
            return;
        static_cast<Impl*>(refcon)->emitFrame(imageBuffer);
    }

    void decodeNal(const uint8_t* nal, size_t len)
    {
        if (!session || len == 0)
            return;

        // Wrap the NAL as AVCC (4-byte big-endian length prefix).
        std::vector<uint8_t> avcc(4 + len);
        avcc[0] = static_cast<uint8_t>((len >> 24) & 0xff);
        avcc[1] = static_cast<uint8_t>((len >> 16) & 0xff);
        avcc[2] = static_cast<uint8_t>((len >> 8) & 0xff);
        avcc[3] = static_cast<uint8_t>(len & 0xff);
        memcpy(avcc.data() + 4, nal, len);

        CMBlockBufferRef block = nullptr;
        OSStatus st = CMBlockBufferCreateWithMemoryBlock(
            kCFAllocatorDefault, nullptr, avcc.size(), kCFAllocatorDefault,
            nullptr, 0, avcc.size(), 0, &block);
        if (st != noErr || !block)
            return;
        CMBlockBufferReplaceDataBytes(avcc.data(), block, 0, avcc.size());

        CMSampleBufferRef sample = nullptr;
        const size_t sampleSize = avcc.size();
        st = CMSampleBufferCreateReady(kCFAllocatorDefault, block, fmt, 1, 0, nullptr,
            1, &sampleSize, &sample);
        if (st == noErr && sample) {
            VTDecodeInfoFlags infoOut = 0;
            VTDecompressionSessionDecodeFrame(session, sample,
                kVTDecodeFrame_EnableAsynchronousDecompression, nullptr, &infoOut);
            CFRelease(sample);
        }
        CFRelease(block);
    }

    void handleNal(const uint8_t* nal, size_t len)
    {
        if (len == 0)
            return;
        int type = nal[0] & 0x1f;
        switch (type) {
        case 7: { // SPS -- the stream resends it every GOP; only rebuild on change.
            std::vector<uint8_t> n(nal, nal + len);
            if (n != sps) {
                sps = std::move(n);
                if (!pps.empty())
                    createSession();
            }
            break;
        }
        case 8: { // PPS
            std::vector<uint8_t> n(nal, nal + len);
            if (n != pps) {
                pps = std::move(n);
                if (!sps.empty())
                    createSession();
            }
            break;
        }
        default: // 5=IDR, 1=non-IDR, others
            decodeNal(nal, len);
            break;
        }
    }

    void feed(const uint8_t* data, size_t len)
    {
        buf.insert(buf.end(), data, data + len);
        if (buf.size() < 4)
            return;

        // Collect start-code offsets (3-byte 00 00 01; a leading 00 makes it 4-byte).
        std::vector<size_t> starts;
        for (size_t i = 0; i + 2 < buf.size(); ++i) {
            if (buf[i] == 0 && buf[i + 1] == 0 && buf[i + 2] == 1)
                starts.push_back(i);
        }
        if (starts.size() < 2) {
            // Keep everything from the first start code (or all) for next feed.
            if (!starts.empty() && starts[0] > 0)
                buf.erase(buf.begin(), buf.begin() + starts[0]);
            return;
        }

        for (size_t k = 0; k + 1 < starts.size(); ++k) {
            size_t nalStart = starts[k] + 3;
            size_t nalEnd = starts[k + 1];
            if (nalEnd > nalStart && buf[nalEnd - 1] == 0) // strip trailing 00 of 4-byte code
                nalEnd--;
            if (nalEnd > nalStart)
                handleNal(buf.data() + nalStart, nalEnd - nalStart);
        }

        // Retain the last (possibly incomplete) NAL for the next feed.
        buf.erase(buf.begin(), buf.begin() + starts.back());
    }
};

AnkerH264Decoder::AnkerH264Decoder() : m_impl(new Impl()) {}
AnkerH264Decoder::~AnkerH264Decoder() { delete m_impl; }

void AnkerH264Decoder::setFrameCallback(FrameCb cb)
{
    std::lock_guard<std::mutex> lock(m_impl->rgbMutex);
    m_impl->cb = std::move(cb);
}

void AnkerH264Decoder::feed(const uint8_t* data, size_t len)
{
    m_impl->feed(data, len);
}

} // namespace AnkerNet
