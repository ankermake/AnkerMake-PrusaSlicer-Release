#ifndef slic3r_GCodeThumbnails_hpp_
#define slic3r_GCodeThumbnails_hpp_

#include "../Point.hpp"
#include "../PrintConfig.hpp"
#include "ThumbnailData.hpp"

#include <vector>
#include <memory>
#include <string_view>

#include <boost/beast/core/detail/base64.hpp>

namespace Slic3r::GCodeThumbnails {

struct CompressedImageBuffer
{
    void       *data { nullptr };
    size_t      size { 0 };
    virtual ~CompressedImageBuffer() {}
    virtual std::string_view tag() const = 0;
};

std::unique_ptr<CompressedImageBuffer> compress_thumbnail(const ThumbnailData &data, GCodeThumbnailsFormat format);

template<typename WriteToOutput, typename ThrowIfCanceledCallback>
inline void export_thumbnails_to_file(ThumbnailsGeneratorCallback &thumbnail_cb, const std::vector<Vec2d> &sizes, GCodeThumbnailsFormat format, WriteToOutput output, ThrowIfCanceledCallback throw_if_canceled)
{
    // Write thumbnails using base64 encoding
    if (thumbnail_cb != nullptr) {
        static constexpr const size_t max_row_length = 78;
        ThumbnailsList thumbnails = thumbnail_cb(ThumbnailsParams{ sizes, true, true, true, true });
        for (const ThumbnailData& data : thumbnails)
            if (data.is_valid()) {
                auto compressed = compress_thumbnail(data, format);
                if (compressed->data && compressed->size) {
                    std::string encoded;
                    encoded.resize(boost::beast::detail::base64::encoded_size(compressed->size));
                    encoded.resize(boost::beast::detail::base64::encode((void*)encoded.data(), (const void*)compressed->data, compressed->size));

                    output((boost::format("\n;\n; %s begin %dx%d %d\n") % compressed->tag() % data.width % data.height % encoded.size()).str().c_str());

                    while (encoded.size() > max_row_length) {
                        output((boost::format("; %s\n") % encoded.substr(0, max_row_length)).str().c_str());
                        encoded = encoded.substr(max_row_length);
                    }

                    if (encoded.size() > 0)
                        output((boost::format("; %s\n") % encoded).str().c_str());

                    output((boost::format("; %s end\n;\n") % compressed->tag()).str().c_str());
                }
                throw_if_canceled();
            }
    }
}


template<typename ImageType, typename StreamType>
ImageType base64ToImage(const std::string& base64) {
    // Decode base64 string to binary data
    std::vector<unsigned char> binaryData;
    binaryData.resize(boost::beast::detail::base64::decoded_size(base64.length()));
    boost::beast::detail::base64::decode(binaryData.data(), base64.c_str(), base64.length());

    // CreateInputStream from binary data
    StreamType stream(binaryData.data(), binaryData.size());

    // Load image from stream
    ImageType image(stream);
    return image;
}

} // namespace Slic3r::GCodeThumbnails

#endif // slic3r_GCodeThumbnails_hpp_
