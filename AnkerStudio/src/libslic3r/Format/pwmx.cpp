#include "pwmx.hpp"
#include "GCode/ThumbnailData.hpp"
#include "SLA/RasterBase.hpp"
#include "libslic3r/SLAPrint.hpp"

#include <sstream>
#include <iostream>
#include <fstream>

#include <boost/algorithm/string/replace.hpp>
#include <boost/log/trivial.hpp>


#define TAG_INTRO "ANYCUBIC\0\0\0\0"
#define TAG_HEADER "HEADER\0\0\0\0\0\0"
#define TAG_PREVIEW "PREVIEW\0\0\0\0\0"
#define TAG_LAYERS "LAYERDEF\0\0\0\0"

#define CFG_LIFT_DISTANCE "LIFT_DISTANCE"
#define CFG_LIFT_SPEED "LIFT_SPEED"
#define CFG_RETRACT_SPEED "RETRACT_SPEED"
#define CFG_DELAY_BEFORE_EXPOSURE "DELAY_BEFORE_EXPOSURE"
#define CFG_BOTTOM_LIFT_SPEED "BOTTOM_LIFT_SPEED"
#define CFG_BOTTOM_LIFT_DISTANCE "BOTTOM_LIFT_DISTANCE"

#define PREV_W 224
#define PREV_H 168
#define PREV_DPI 42

#define LAYER_SIZE_ESTIMATE (32 * 1024)

namespace Slic3r {

static void pwx_get_pixel_span(const std::uint8_t* ptr, const std::uint8_t* end,
                               std::uint8_t& pixel, size_t& span_len)
{
    size_t max_len;

    span_len = 0;
    pixel = (*ptr) & 0xF0;
    // the maximum length of the span depends on the pixel color
    max_len = (pixel == 0 || pixel == 0xF0) ? 0xFFF : 0xF;
    while (ptr < end && span_len < max_len && ((*ptr) & 0xF0) == pixel) {
        span_len++;
        ptr++;
    }
}

struct PWXRasterEncoder
{
    sla::EncodedRaster operator()(const void *ptr,
                                  size_t      w,
                                  size_t      h,
                                  size_t      num_components)
    {
        std::vector<uint8_t> dst;
        size_t               span_len;
        std::uint8_t         pixel;
        auto                 size = w * h * num_components;
        dst.reserve(size);

        const std::uint8_t *src = reinterpret_cast<const std::uint8_t *>(ptr);
        const std::uint8_t *src_end = src + size;
        while (src < src_end) {
            pwx_get_pixel_span(src, src_end, pixel, span_len);
            src += span_len;
            // fully transparent of fully opaque pixel
            if (pixel == 0 || pixel == 0xF0) {
                pixel = pixel | (span_len >> 8);
                std::copy(&pixel, (&pixel) + 1, std::back_inserter(dst));
                pixel = span_len & 0xFF;
                std::copy(&pixel, (&pixel) + 1, std::back_inserter(dst));
            }
            // antialiased pixel
            else {
                pixel = pixel | span_len;
                std::copy(&pixel, (&pixel) + 1, std::back_inserter(dst));
            }
        }

        return sla::EncodedRaster(std::move(dst), "pwx");
    }
};

using ConfMap = std::map<std::string, std::string>;

typedef struct pwmx_format_intro
{
    char          tag[12];
    std::uint32_t version;  // value 1
    std::uint32_t area_num; // unknown - usually 4
    std::uint32_t header_data_offset;
    std::float_t  intro24; // unknown - usually 0
    std::uint32_t preview_data_offset;
    std::float_t  intro32; // unknown
    std::uint32_t layer_data_offset;
    std::float_t  intro40; // unknown
    std::uint32_t image_data_offset;
} pwmx_format_intro;

typedef struct pwmx_format_header
{
    char          tag[12];
    std::uint32_t payload_size;
    std::float_t  pixel_size_um;
    std::float_t  layer_height_mm;
    std::float_t  exposure_time_s;
    std::float_t  delay_before_exposure_s;
    std::float_t  bottom_exposure_time_s;
    std::float_t  bottom_layer_count;
    std::float_t  lift_distance_mm;
    std::float_t  lift_speed_mms;
    std::float_t  retract_speed_mms;
    std::float_t  volume_ml;
    std::uint32_t antialiasing;
    std::uint32_t res_x;
    std::uint32_t res_y;
    std::float_t  weight_g;
    std::float_t  price;
    std::uint32_t price_currency;
    std::uint32_t per_layer_override; // ? unknown meaning ?
    std::uint32_t print_time_s;
    std::uint32_t transition_layer_count;
    std::uint32_t unknown; // ? usually 0 ?

} pwmx_format_header;

typedef struct pwmx_format_preview
{
    char          tag[12];
    std::uint32_t payload_size;
    std::uint32_t preview_w;
    std::uint32_t preview_dpi;
    std::uint32_t preview_h;
    // raw image data in BGR565 format
     std::uint8_t pixels[PREV_W * PREV_H * 2];
} pwmx_format_preview;

typedef struct pwmx_format_layers_header
{
    char          tag[12];
    std::uint32_t payload_size;
    std::uint32_t layer_count;
} pwmx_format_layers_header;

typedef struct pwmx_format_layer
{
    std::uint32_t image_offset;
    std::uint32_t image_size;
    std::float_t  lift_distance_mm;
    std::float_t  lift_speed_mms;
    std::float_t  exposure_time_s;
    std::float_t  layer_height_mm;
    std::float_t  layer44; // unkown - usually 0
    std::float_t  layer48; // unkown - usually 0
} pwmx_format_layer;

typedef struct pwmx_format_misc
{
    std::float_t bottom_layer_height_mm;
    std::float_t bottom_lift_distance_mm;
    std::float_t bottom_lift_speed_mms;

} pwmx_format_misc;

class PwmxFormatConfigDef : public ConfigDef
{
public:
    PwmxFormatConfigDef()
    {
        add(CFG_LIFT_DISTANCE, coFloat);
        add(CFG_LIFT_SPEED, coFloat);
        add(CFG_RETRACT_SPEED, coFloat);
        add(CFG_DELAY_BEFORE_EXPOSURE, coFloat);
        add(CFG_BOTTOM_LIFT_DISTANCE, coFloat);
        add(CFG_BOTTOM_LIFT_SPEED, coFloat);
    }
};

class PwmxFormatDynamicConfig : public DynamicConfig
{
public:
    PwmxFormatDynamicConfig(){};
    const ConfigDef *def() const override { return &config_def; }

private:
    PwmxFormatConfigDef config_def;
};

namespace {

std::float_t get_cfg_value_f(const DynamicConfig &cfg,
                             const std::string   &key,
                             const std::float_t  &def = 0.f)
{
    if (cfg.has(key)) {
        if (auto opt = cfg.option(key))
            return opt->getFloat();
    }

    return def;
}

int get_cfg_value_i(const DynamicConfig &cfg,
                    const std::string   &key,
                    const int           &def = 0)
{
    if (cfg.has(key)) {
        if (auto opt = cfg.option(key))
            return opt->getInt();
    }

    return def;
}

template<class T> void crop_value(T &val, T val_min, T val_max)
{
    if (val < val_min) {
        val = val_min;
    } else if (val > val_max) {
        val = val_max;
    }
}

void fill_preview(pwmx_format_preview &p,
                  pwmx_format_misc   &/*m*/,
                  const ThumbnailsList &thumbnails)
{

    p.preview_w    = PREV_W;
    p.preview_h    = PREV_H;
    p.preview_dpi  = PREV_DPI;
    p.payload_size = sizeof(p) - sizeof(p.tag) - sizeof(p.payload_size);
                     
    std::memset(p.pixels, 0 , sizeof(p.pixels));
    if (!thumbnails.empty()) {
        std::uint32_t dst_index;
        std::uint32_t i = 0;
        size_t len;
        size_t pixel_x = 0;
        auto t = thumbnails[0]; //use the first thumbnail
        len = t.pixels.size();
        //sanity check        
        if (len != PREV_W * PREV_H * 4)  {
            printf("incorrect thumbnail size. expected %ix%i\n", PREV_W, PREV_H);
            return;
        }
        // rearange pixels: they seem to be stored from bottom to top.
        dst_index = (PREV_W * (PREV_H - 1) * 2);
        while (i < len) {
            std::uint32_t pixel;
            std::uint32_t r = t.pixels[i++];
            std::uint32_t g = t.pixels[i++];
            std::uint32_t b = t.pixels[i++];
            i++; // Alpha
            // convert to BGRA565
            pixel = ((b >> 3) << 11) | ((g >>2) << 5) | (r >> 3);
            p.pixels[dst_index++] = pixel & 0xFF;
            p.pixels[dst_index++] = (pixel >> 8) & 0xFF;
            pixel_x++;
            if (pixel_x == PREV_W) {
                pixel_x = 0;
                dst_index -= (PREV_W * 4);
            }
        }
    }
}


void fill_header(pwmx_format_header &h,
                 pwmx_format_misc   &m,
                 const SLAPrint     &print,
                 std::uint32_t       layer_count)
{
    CNumericLocalesSetter locales_setter;

    std::float_t bottle_weight_g;
    std::float_t bottle_volume_ml;
    std::float_t bottle_cost;
    std::float_t material_density;
    auto        &cfg     = print.full_print_config();
    auto         mat_opt = cfg.option("material_notes");
    std::string  mnotes  = mat_opt? cfg.option("material_notes")->serialize() : "";
    // create a config parser from the material notes
    Slic3r::PwmxFormatDynamicConfig mat_cfg;
    SLAPrintStatistics              stats = print.print_statistics();

    // sanitize the string config
    boost::replace_all(mnotes, "\\n", "\n");
    boost::replace_all(mnotes, "\\r", "\r");
    mat_cfg.load_from_ini_string(mnotes,
                                 ForwardCompatibilitySubstitutionRule::Enable);

    h.layer_height_mm        = get_cfg_value_f(cfg, "layer_height");
    m.bottom_layer_height_mm = get_cfg_value_f(cfg, "initial_layer_height");
    h.exposure_time_s        = get_cfg_value_f(cfg, "exposure_time");
    h.bottom_exposure_time_s = get_cfg_value_f(cfg, "initial_exposure_time");
    h.bottom_layer_count =     get_cfg_value_i(cfg, "faded_layers");
    if (layer_count < h.bottom_layer_count) {
        h.bottom_layer_count = layer_count;
    }
    h.res_x     = get_cfg_value_i(cfg, "display_pixels_x");
    h.res_y     = get_cfg_value_i(cfg, "display_pixels_y");
    bottle_weight_g = get_cfg_value_f(cfg, "bottle_weight") * 1000.0f;
    bottle_volume_ml = get_cfg_value_f(cfg, "bottle_volume");
    bottle_cost = get_cfg_value_f(cfg, "bottle_cost");
    material_density = bottle_weight_g / bottle_volume_ml;

    h.volume_ml = (stats.objects_used_material + stats.support_used_material) / 1000;
    h.weight_g           = h.volume_ml * material_density;
    h.price              = (h.volume_ml * bottle_cost) /  bottle_volume_ml;
    h.price_currency     = '$';
    h.antialiasing       = 1;
    h.per_layer_override = 0;

    // TODO - expose these variables to the UI rather than using material notes
    h.delay_before_exposure_s = get_cfg_value_f(mat_cfg, CFG_DELAY_BEFORE_EXPOSURE, 0.5f);
    crop_value(h.delay_before_exposure_s, 0.0f, 1000.0f);

    h.lift_distance_mm = get_cfg_value_f(mat_cfg, CFG_LIFT_DISTANCE, 8.0f);
    crop_value(h.lift_distance_mm, 0.0f, 100.0f);

    if (mat_cfg.has(CFG_BOTTOM_LIFT_DISTANCE)) {
        m.bottom_lift_distance_mm = get_cfg_value_f(mat_cfg,
                                                    CFG_BOTTOM_LIFT_DISTANCE,
                                                    8.0f);
        crop_value(h.lift_distance_mm, 0.0f, 100.0f);
    } else {
        m.bottom_lift_distance_mm = h.lift_distance_mm;
    }

    h.lift_speed_mms = get_cfg_value_f(mat_cfg, CFG_LIFT_SPEED, 2.0f);
    crop_value(m.bottom_lift_speed_mms, 0.1f, 20.0f);

    if (mat_cfg.has(CFG_BOTTOM_LIFT_SPEED)) {
        m.bottom_lift_speed_mms = get_cfg_value_f(mat_cfg, CFG_BOTTOM_LIFT_SPEED, 2.0f);
        crop_value(m.bottom_lift_speed_mms, 0.1f, 20.0f);
    } else {
        m.bottom_lift_speed_mms = h.lift_speed_mms;
    }

    h.retract_speed_mms = get_cfg_value_f(mat_cfg, CFG_RETRACT_SPEED, 3.0f);
    crop_value(h.lift_speed_mms, 0.1f, 20.0f);

    h.print_time_s = (h.bottom_layer_count * h.bottom_exposure_time_s) +
                     ((layer_count - h.bottom_layer_count) *
                      h.exposure_time_s) +
                     (layer_count * h.lift_distance_mm / h.retract_speed_mms) +
                     (layer_count * h.lift_distance_mm / h.lift_speed_mms) +
                     (layer_count * h.delay_before_exposure_s);


    h.payload_size  = sizeof(h) - sizeof(h.tag) - sizeof(h.payload_size);
    h.pixel_size_um = 50;
}

} // namespace

std::unique_ptr<sla::RasterBase> PwmxArchive::create_raster() const
{
    sla::Resolution     res;
    sla::PixelDim       pxdim;
    std::array<bool, 2> mirror;

    double w  = m_cfg.display_width.getFloat();
    double h  = m_cfg.display_height.getFloat();
    auto   pw = size_t(m_cfg.display_pixels_x.getInt());
    auto   ph = size_t(m_cfg.display_pixels_y.getInt());

    mirror[X] = m_cfg.display_mirror_x.getBool();
    mirror[Y] = m_cfg.display_mirror_y.getBool();

    auto                         ro = m_cfg.display_orientation.getInt();
    sla::RasterBase::Orientation orientation =
        ro == sla::RasterBase::roPortrait ? sla::RasterBase::roPortrait :
                                            sla::RasterBase::roLandscape;

    if (orientation == sla::RasterBase::roPortrait) {
        std::swap(w, h);
        std::swap(pw, ph);
    }

    res   = sla::Resolution{pw, ph};
    pxdim = sla::PixelDim{w / pw, h / ph};
    sla::RasterBase::Trafo tr{orientation, mirror};

    double gamma = m_cfg.gamma_correction.getFloat();

    return sla::create_raster_grayscale_aa(res, pxdim, gamma, tr);
}

sla::RasterEncoder PwmxArchive::get_encoder() const
{
    return PWXRasterEncoder{};
}

// Endian safe write of little endian 32bit ints
static void pwmx_write_int32(std::ofstream &out, std::uint32_t val)
{
    const char i1 = (val & 0xFF);
    const char i2 = (val >> 8) & 0xFF;
    const char i3 = (val >> 16) & 0xFF;
    const char i4 = (val >> 24) & 0xFF;

    out.write((const char *) &i1, 1);
    out.write((const char *) &i2, 1);
    out.write((const char *) &i3, 1);
    out.write((const char *) &i4, 1);
}
static void pwmx_write_float(std::ofstream &out, std::float_t val)
{
    std::uint32_t *f = (std::uint32_t *) &val;
    pwmx_write_int32(out, *f);
}

static void pwmx_write_intro(std::ofstream &out, pwmx_format_intro &i)
{
    out.write(TAG_INTRO, sizeof(i.tag));
    pwmx_write_int32(out, i.version);
    pwmx_write_int32(out, i.area_num);
    pwmx_write_int32(out, i.header_data_offset);
    pwmx_write_int32(out, i.intro24);
    pwmx_write_int32(out, i.preview_data_offset);
    pwmx_write_int32(out, i.intro32);
    pwmx_write_int32(out, i.layer_data_offset);
    pwmx_write_int32(out, i.intro40);
    pwmx_write_int32(out, i.image_data_offset);
}

static void pwmx_write_header(std::ofstream &out, pwmx_format_header &h)
{
    out.write(TAG_HEADER, sizeof(h.tag));
    pwmx_write_int32(out, h.payload_size);
    pwmx_write_float(out, h.pixel_size_um);
    pwmx_write_float(out, h.layer_height_mm);
    pwmx_write_float(out, h.exposure_time_s);
    pwmx_write_float(out, h.delay_before_exposure_s);
    pwmx_write_float(out, h.bottom_exposure_time_s);
    pwmx_write_float(out, h.bottom_layer_count);
    pwmx_write_float(out, h.lift_distance_mm);
    pwmx_write_float(out, h.lift_speed_mms);
    pwmx_write_float(out, h.retract_speed_mms);
    pwmx_write_float(out, h.volume_ml);
    pwmx_write_int32(out, h.antialiasing);
    pwmx_write_int32(out, h.res_x);
    pwmx_write_int32(out, h.res_y);
    pwmx_write_float(out, h.weight_g);
    pwmx_write_float(out, h.price);
    pwmx_write_int32(out, h.price_currency);
    pwmx_write_int32(out, h.per_layer_override);
    pwmx_write_int32(out, h.print_time_s);
    pwmx_write_int32(out, h.transition_layer_count);
    pwmx_write_int32(out, h.unknown);
}

static void pwmx_write_preview(std::ofstream &out, pwmx_format_preview &p)
{
    out.write(TAG_PREVIEW, sizeof(p.tag));
    pwmx_write_int32(out, p.payload_size);
    pwmx_write_int32(out, p.preview_w);
    pwmx_write_int32(out, p.preview_dpi);
    pwmx_write_int32(out, p.preview_h);
    out.write((const char*) p.pixels, sizeof(p.pixels));
}

static void pwmx_write_layers_header(std::ofstream &out, pwmx_format_layers_header &h)
{
    out.write(TAG_LAYERS, sizeof(h.tag));
    pwmx_write_int32(out, h.payload_size);
    pwmx_write_int32(out, h.layer_count);
}

static void pwmx_write_layer(std::ofstream &out, pwmx_format_layer &l)
{
    pwmx_write_int32(out, l.image_offset);
    pwmx_write_int32(out, l.image_size);
    pwmx_write_float(out, l.lift_distance_mm);
    pwmx_write_float(out, l.lift_speed_mms);
    pwmx_write_float(out, l.exposure_time_s);
    pwmx_write_float(out, l.layer_height_mm);
    pwmx_write_float(out, l.layer44);
    pwmx_write_float(out, l.layer48);
}

void PwmxArchive::export_print(const std::string     fname,
                               const SLAPrint       &print,
                               const ThumbnailsList &thumbnails,
                               const std::string    &/*projectname*/)
{
    std::uint32_t layer_count = m_layers.size();

    pwmx_format_intro         intro = {};
    pwmx_format_header        header = {};
    pwmx_format_preview       preview = {};
    pwmx_format_layers_header layers_header = {};
    pwmx_format_misc          misc = {};
    std::vector<uint8_t>      layer_images;
    std::uint32_t             image_offset;

    intro.version             = 1;
    intro.area_num            = 4;
    intro.header_data_offset  = sizeof(intro);
    intro.preview_data_offset = sizeof(intro) + sizeof(header);
    intro.layer_data_offset   = intro.preview_data_offset + sizeof(preview);
    intro.image_data_offset = intro.layer_data_offset +
                              sizeof(layers_header) +
                              (sizeof(pwmx_format_layer) * layer_count);

    fill_header(header, misc, print, layer_count);
    fill_preview(preview, misc, thumbnails);

    try {
        // open the file and write the contents
        std::ofstream out;
        out.open(fname, std::ios::binary | std::ios::out | std::ios::trunc);
        pwmx_write_intro(out, intro);
        pwmx_write_header(out, header);
        pwmx_write_preview(out, preview);

        layers_header.payload_size = intro.image_data_offset - intro.layer_data_offset -
                        sizeof(layers_header.tag)  - sizeof(layers_header.payload_size);
        layers_header.layer_count = layer_count;
        pwmx_write_layers_header(out, layers_header);

        //layers
        layer_images.reserve(layer_count * LAYER_SIZE_ESTIMATE);
        image_offset = intro.image_data_offset;
        size_t i = 0;
        for (const sla::EncodedRaster &rst : m_layers) {
            pwmx_format_layer l;
            std::memset(&l, 0, sizeof(l));
            l.image_offset = image_offset;
            l.image_size = rst.size();
            if (i < header.bottom_layer_count) {
                l.exposure_time_s = header.bottom_exposure_time_s;
                l.layer_height_mm = misc.bottom_layer_height_mm;
                l.lift_distance_mm = misc.bottom_lift_distance_mm;
                l.lift_speed_mms = misc.bottom_lift_speed_mms;
            } else {
                l.exposure_time_s = header.exposure_time_s;
                l.layer_height_mm = header.layer_height_mm;
                l.lift_distance_mm = header.lift_distance_mm;
                l.lift_speed_mms = header.lift_speed_mms;
            }
            image_offset += l.image_size;
            pwmx_write_layer(out, l);
            // add the rle encoded layer image into the buffer
            const char* img_start = reinterpret_cast<const char*>(rst.data());
            const char* img_end = img_start + rst.size();
            std::copy(img_start, img_end, std::back_inserter(layer_images));
            i++;
        }
        const char* img_buffer = reinterpret_cast<const char*>(layer_images.data());
        out.write(img_buffer, layer_images.size());
        out.close();
    } catch(std::exception& e) {
        BOOST_LOG_TRIVIAL(error) << e.what();
        // Rethrow the exception
        throw;
    }

}

} // namespace Slic3r
