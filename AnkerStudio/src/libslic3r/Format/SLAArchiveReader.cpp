#include "SLAArchiveReader.hpp"
#include "SL1.hpp"
#include "SL1_SVG.hpp"
#include "I18N.hpp"

#include "libslic3r/SlicesToTriangleMesh.hpp"

#include <boost/filesystem/path.hpp>
#include <boost/algorithm/string.hpp>


#include <array>
#include <map>

namespace Slic3r {

namespace {

// Factory function that returns an implementation of SLAArchiveReader.
using ArchiveFactory = std::function<
    std::unique_ptr<SLAArchiveReader>(const std::string       &fname,
                                      SLAImportQuality         quality,
                                      const ProgrFn & progr)>;

// Entry in the global registry of readable archive formats.
struct ArchiveEntry {
    const char *descr;
    std::vector<const char *> extensions;
    ArchiveFactory factoryfn;
};

// This is where the readable archive formats are registered.
static const std::map<std::string, ArchiveEntry> REGISTERED_ARCHIVES {
    {
        "SL1",
        { L("SL1 / SL1S archive files"), {"sl1", "sl1s", "zip"},
         [] (const std::string &fname, SLAImportQuality quality, const ProgrFn &progr) { return std::make_unique<SL1Reader>(fname, quality, progr); } }
    },
    {
        "SL1SVG",
        { L("SL1SVG archive files"), {"sl1_svg"/*, "zip"*/}, // also a zip but unnecessary hassle to implement single extension for multiple archives
         [] (const std::string &fname, SLAImportQuality quality, const ProgrFn &progr) { return std::make_unique<SL1_SVGReader>(fname, quality, progr); }}
    },
    // TODO: pwmx and future others.
};

} // namespace

std::unique_ptr<SLAArchiveReader> SLAArchiveReader::create(
    const std::string       &fname,
    const std::string       &format_id,
    SLAImportQuality         quality,
    const ProgrFn & progr)
{
    // Create an instance of SLAArchiveReader using the registered archive
    // reader implementations.
    // If format_id is specified and valid, that archive format will be
    // preferred. When format_id is emtpy, the file extension is compared
    // with the advertised extensions of registered readers and the first
    // match will be used.

    std::string ext = boost::filesystem::path(fname).extension().string();
    boost::algorithm::to_lower(ext);

    std::unique_ptr<SLAArchiveReader> ret;

    auto arch_from = REGISTERED_ARCHIVES.begin();
    auto arch_to   = REGISTERED_ARCHIVES.end();

    auto arch_it = REGISTERED_ARCHIVES.find(format_id);
    if (arch_it != REGISTERED_ARCHIVES.end()) {
        arch_from = arch_it;
        arch_to   = arch_it;
    }

    if (!ext.empty()) {
        if (ext.front() == '.')
            ext.erase(ext.begin());

        auto extcmp = [&ext](const auto &e) { return e == ext; };

        for (auto it = arch_from; it != arch_to; ++it) {
            const auto &[format_id, entry] = *it;
            if (std::any_of(entry.extensions.begin(), entry.extensions.end(), extcmp))
                ret = entry.factoryfn(fname, quality, progr);
        }
    }

    return ret;
}

const std::vector<const char *> &SLAArchiveReader::registered_archives()
{
    static std::vector<const char*> archnames;

    if (archnames.empty()) {
        archnames.reserve(REGISTERED_ARCHIVES.size());

        for (auto &[name, _] : REGISTERED_ARCHIVES)
            archnames.emplace_back(name.c_str());
    }

    return archnames;
}

std::vector<const char *> SLAArchiveReader::get_extensions(const char *archtype)
{
    auto it = REGISTERED_ARCHIVES.find(archtype);

    if (it != REGISTERED_ARCHIVES.end())
        return it->second.extensions;

    return {};
}

const char *SLAArchiveReader::get_description(const char *archtype)
{
    auto it = REGISTERED_ARCHIVES.find(archtype);

    if (it != REGISTERED_ARCHIVES.end())
        return it->second.descr;

    return nullptr;
}

struct SliceParams { double layerh = 0., initial_layerh = 0.; };

static SliceParams get_slice_params(const DynamicPrintConfig &cfg)
{
    auto *opt_layerh = cfg.option<ConfigOptionFloat>("layer_height");
    auto *opt_init_layerh = cfg.option<ConfigOptionFloat>("initial_layer_height");

    if (!opt_layerh || !opt_init_layerh)
        throw MissingProfileError("Invalid SL1 / SL1S file");

    return SliceParams{opt_layerh->getFloat(), opt_init_layerh->getFloat()};
}

ConfigSubstitutions import_sla_archive(const std::string       &zipfname,
                                       const std::string       &format_id,
                                       indexed_triangle_set    &out,
                                       DynamicPrintConfig      &profile,
                                       SLAImportQuality         quality,
                                       const ProgrFn & progr)
{
    ConfigSubstitutions ret;

    if (auto reader = SLAArchiveReader::create(zipfname, format_id, quality, progr)) {
        std::vector<ExPolygons> slices;
        ret = reader->read(slices, profile);

        SliceParams slicp = get_slice_params(profile);

        if (!slices.empty())
            out = slices_to_mesh(slices, 0, slicp.layerh, slicp.initial_layerh);

    } else {
        throw ReaderUnimplementedError("Reader unimplemented");
    }

    return ret;
}

ConfigSubstitutions import_sla_archive(const std::string  &zipfname,
                                       const std::string  &format_id,
                                       DynamicPrintConfig &out)
{
    ConfigSubstitutions ret;

    if (auto reader = SLAArchiveReader::create(zipfname, format_id)) {
        ret = reader->read(out);
    } else {
        throw ReaderUnimplementedError("Reader unimplemented");
    }

    return ret;
}

} // namespace Slic3r
