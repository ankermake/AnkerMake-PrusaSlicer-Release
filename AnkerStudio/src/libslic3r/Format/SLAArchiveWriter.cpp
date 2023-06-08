#include "SLAArchiveWriter.hpp"

#include "SL1.hpp"
#include "SL1_SVG.hpp"
#include "pwmx.hpp"

#include "libslic3r/libslic3r.h"

#include <string>
#include <map>
#include <memory>
#include <tuple>

namespace Slic3r {

using ArchiveFactory = std::function<std::unique_ptr<SLAArchiveWriter>(const SLAPrinterConfig&)>;

struct ArchiveEntry {
    const char *ext;
    ArchiveFactory factoryfn;
};

static const std::map<std::string, ArchiveEntry> REGISTERED_ARCHIVES {
    {
        "SL1",
        { "sl1",  [] (const auto &cfg) { return std::make_unique<SL1Archive>(cfg); } }
    },
    {
        "SL1SVG",
        { "sl1_svg",  [] (const auto &cfg) { return std::make_unique<SL1_SVGArchive>(cfg); } }
    },
    {
        "SL2",
        { "sl1_svg",  [] (const auto &cfg) { return std::make_unique<SL1_SVGArchive>(cfg); } }
    },
    {
        "pwmx",
        { "pwmx", [] (const auto &cfg) { return std::make_unique<PwmxArchive>(cfg); } }
    }
};

std::unique_ptr<SLAArchiveWriter>
SLAArchiveWriter::create(const std::string &archtype, const SLAPrinterConfig &cfg)
{
    auto entry = REGISTERED_ARCHIVES.find(archtype);

    if (entry != REGISTERED_ARCHIVES.end())
        return entry->second.factoryfn(cfg);

    return nullptr;
}

const std::vector<const char*>& SLAArchiveWriter::registered_archives()
{
    static std::vector<const char*> archnames;

    if (archnames.empty()) {
        archnames.reserve(REGISTERED_ARCHIVES.size());

        for (auto &[name, _] : REGISTERED_ARCHIVES)
            archnames.emplace_back(name.c_str());
    }

    return archnames;
}

const char *SLAArchiveWriter::get_extension(const char *archtype)
{
    constexpr const char* DEFAULT_EXT = "zip";

    auto entry = REGISTERED_ARCHIVES.find(archtype);
    if (entry != REGISTERED_ARCHIVES.end())
        return entry->second.ext;

    return DEFAULT_EXT;
}

} // namespace Slic3r
