#include "Thumbnails.hpp"
#include "../miniz_extension.hpp"

#include <qoi/qoi.h>
#include <jpeglib.h>
#include <jerror.h>

namespace Slic3r::GCodeThumbnails {

using namespace std::literals;

struct CompressedPNG : CompressedImageBuffer 
{
    ~CompressedPNG() override { if (data) mz_free(data); }
    std::string_view tag() const override { return "thumbnail"sv; }
};

struct CompressedJPG : CompressedImageBuffer
{
    ~CompressedJPG() override { free(data); }
    std::string_view tag() const override { return "thumbnail_JPG"sv; }
};

struct CompressedQOI : CompressedImageBuffer 
{
    ~CompressedQOI() override { free(data); }
    std::string_view tag() const override { return "thumbnail_QOI"sv; }
};

std::unique_ptr<CompressedImageBuffer> compress_thumbnail_png(const ThumbnailData &data)
{
    auto out = std::make_unique<CompressedPNG>();
    out->data = tdefl_write_image_to_png_file_in_memory_ex((const void*)data.pixels.data(), data.width, data.height, 4, &out->size, MZ_DEFAULT_LEVEL, 1);
    return out;
}

std::unique_ptr<CompressedImageBuffer> compress_thumbnail_jpg(const ThumbnailData& data)
{
    // Take vector of RGBA pixels and flip the image vertically
    std::vector<unsigned char> rgba_pixels(data.pixels.size());
    const unsigned int row_size = data.width * 4;
    for (unsigned int y = 0; y < data.height; ++y) {
        ::memcpy(rgba_pixels.data() + (data.height - y - 1) * row_size, data.pixels.data() + y * row_size, row_size);
    }

    // Store pointers to scanlines start for later use
    std::vector<unsigned char*> rows_ptrs;
    rows_ptrs.reserve(data.height);
    for (unsigned int y = 0; y < data.height; ++y) {
        rows_ptrs.emplace_back(&rgba_pixels[y * row_size]);
    }

    std::vector<unsigned char> compressed_data(data.pixels.size());
    unsigned char* compressed_data_ptr = compressed_data.data();
    unsigned long compressed_data_size = data.pixels.size();

    jpeg_error_mgr err;
    jpeg_compress_struct info;
    info.err = jpeg_std_error(&err);
    jpeg_create_compress(&info);
    jpeg_mem_dest(&info, &compressed_data_ptr, &compressed_data_size);

    info.image_width = data.width;
    info.image_height = data.height;
    info.input_components = 4;
    info.in_color_space = JCS_EXT_RGBA;

    jpeg_set_defaults(&info);
    jpeg_set_quality(&info, 85, TRUE);
    jpeg_start_compress(&info, TRUE);

    jpeg_write_scanlines(&info, rows_ptrs.data(), data.height);
    jpeg_finish_compress(&info);
    jpeg_destroy_compress(&info);

    // FIXME -> Add error checking

    auto out = std::make_unique<CompressedJPG>();
    out->data = malloc(compressed_data_size);
    out->size = size_t(compressed_data_size);
    ::memcpy(out->data, (const void*)compressed_data.data(), out->size);
    return out;
}

std::unique_ptr<CompressedImageBuffer> compress_thumbnail_qoi(const ThumbnailData &data)
{
    qoi_desc desc;
    desc.width      = data.width;
    desc.height     = data.height;
    desc.channels   = 4;
    desc.colorspace = QOI_SRGB;

    // Take vector of RGBA pixels and flip the image vertically
    std::vector<uint8_t> rgba_pixels(data.pixels.size() * 4);
    size_t row_size = data.width * 4;
    for (size_t y = 0; y < data.height; ++ y)
        memcpy(rgba_pixels.data() + (data.height - y - 1) * row_size, data.pixels.data() + y * row_size, row_size);
    
    auto out = std::make_unique<CompressedQOI>();
    int  size;
    out->data = qoi_encode((const void*)rgba_pixels.data(), &desc, &size);
    out->size = size;
    return out;
}

std::unique_ptr<CompressedImageBuffer> compress_thumbnail(const ThumbnailData &data, GCodeThumbnailsFormat format)
{
    switch (format) {
        case GCodeThumbnailsFormat::PNG:
        default:
            return compress_thumbnail_png(data);
        case GCodeThumbnailsFormat::JPG:
            return compress_thumbnail_jpg(data);
        case GCodeThumbnailsFormat::QOI:
            return compress_thumbnail_qoi(data);
    }
}

std::unique_ptr<CompressedImageBuffer> compress_akpic(const post_gcode::picData& data)
{   //only jpeg
    // Take vector of RGBA pixels and flip the image vertically
    std::vector<unsigned char> rgba_pixels(data.pixels.size());
    const unsigned int row_size = data.width * 4;
    for (unsigned int y = 0; y < data.height; ++y) {
        ::memcpy(rgba_pixels.data() + (data.height - y - 1) * row_size, data.pixels.data() + y * row_size, row_size);
    }

    // Store pointers to scanlines start for later use
    std::vector<unsigned char*> rows_ptrs;
    rows_ptrs.reserve(data.height);
    for (unsigned int y = 0; y < data.height; ++y) {
        rows_ptrs.emplace_back(&rgba_pixels[y * row_size]);
    }

    std::vector<unsigned char> compressed_data(data.pixels.size());
    unsigned char* compressed_data_ptr = compressed_data.data();
    unsigned long compressed_data_size = data.pixels.size();

    jpeg_error_mgr err;
    jpeg_compress_struct info;
    info.err = jpeg_std_error(&err);
    jpeg_create_compress(&info);
    jpeg_mem_dest(&info, &compressed_data_ptr, &compressed_data_size);

    info.image_width = data.width;
    info.image_height = data.height;
    info.input_components = 4;
    info.in_color_space = JCS_EXT_RGBA;

    jpeg_set_defaults(&info);
    jpeg_set_quality(&info, 85, TRUE);
    jpeg_start_compress(&info, TRUE);

    jpeg_write_scanlines(&info, rows_ptrs.data(), data.height);
    jpeg_finish_compress(&info);
    jpeg_destroy_compress(&info);

    // FIXME -> Add error checking

    auto out = std::make_unique<CompressedJPG>();
    out->data = malloc(compressed_data_size);
    out->size = size_t(compressed_data_size);
    ::memcpy(out->data, (const void*)compressed_data.data(), out->size);
    return out;
}

} // namespace Slic3r::GCodeThumbnails

post_gcode::processAiPicture::processAiPicture(std::string fileName, std::string fullPath, unsigned int totalLayers, unsigned int total_picture)
{
    this->m_head.magic = 0x3DA13DA1;
    std::copy(fileName.begin(), fileName.end(), this->m_head.relate_gcode_name); //TODO: is need check
    this->m_head.version_num = 1;//TODO: need version name
    this->m_head.total_layer = totalLayers;
    this->m_head.total_picture = total_picture;
    this->m_head.file_size = 128;
    m_outFile.open(fullPath, ios::out | ios::binary);
    m_outFile.write(this->m_head.a, sizeof(this->m_head));
    m_outFile.flush();
}

post_gcode::processAiPicture::~processAiPicture()
{
    this->m_outFile.seekp(0, ios::beg);
    this->m_outFile.write(this->m_head.a, sizeof(this->m_head));
    this->m_outFile.flush();
    this->m_outFile.close();
}

void post_gcode::processAiPicture::writeImage(imageHead iHead, imgInfo iInfo)
{
    m_outFile.write(iHead.a, sizeof(iHead));
    this->m_head.file_size += 64;
    m_outFile.flush();
    m_outFile.write(iInfo.data, iInfo.size);
    this->m_head.file_size += iInfo.size;
    m_outFile.flush();
}
