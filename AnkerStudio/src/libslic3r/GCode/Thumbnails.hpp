#ifndef slic3r_GCodeThumbnails_hpp_
#define slic3r_GCodeThumbnails_hpp_

#include "../Point.hpp"
#include "../PrintConfig.hpp"
#include "ThumbnailData.hpp"

#include <vector>
#include <memory>
#include <string_view>
#include <iostream>
#include <fstream>
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
std::unique_ptr<CompressedImageBuffer> compress_akpic(const post_gcode::picData &data);

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
    ImageType image;
    if (ImageType::CanRead(stream))
        image = ImageType(stream);

    return image;
}

} // namespace Slic3r::GCodeThumbnails


namespace post_gcode {
    using namespace std;

    union fileHead {
        char a[128] = { 0 };
        struct {
            unsigned int  magic;                
            unsigned int  crc32;                
            unsigned int  version_num;          
            unsigned int  total_picture;        
            unsigned int  total_layer;          
            unsigned int  file_size;            
            char relate_gcode_name[64];         
        };
    };


    union imageHead {
        char a[64] = { 0 };
        struct {
            float         percentage;        
            unsigned int  layer_num;         
            unsigned int  file_size;         
            float         roi_info[4];       
            char name[32];                   
        };
    };

    struct imgInfo
    {
        char* data;
        size_t size;
    };

    class processAiPicture
    {
        std::ofstream m_outFile;
        fileHead m_head;
    public:
        processAiPicture(std::string fileName, std::string fullPath, unsigned int totalLayers, unsigned int total_picture);
        ~processAiPicture();
        void writeImage(imageHead iHead, imgInfo iInfo);

    };

}


#endif // slic3r_GCodeThumbnails_hpp_

