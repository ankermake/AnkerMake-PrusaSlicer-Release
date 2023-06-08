#include <catch2/catch.hpp>
#include <test_utils.hpp>

#include "libslic3r/SLAPrint.hpp"
#include "libslic3r/TriangleMesh.hpp"
#include "libslic3r/Format/SLAArchiveWriter.hpp"
#include "libslic3r/Format/SLAArchiveReader.hpp"

#include <boost/filesystem.hpp>

using namespace Slic3r;

TEST_CASE("Archive export test", "[sla_archives]") {
    for (const char * pname : {"20mm_cube", "extruder_idler"})
    for (auto &archname : SLAArchiveWriter::registered_archives()) {
        INFO(std::string("Testing archive type: ") + archname + " -- writing...");
        SLAPrint print;
        SLAFullPrintConfig fullcfg;

        auto m = Model::read_from_file(TEST_DATA_DIR PATH_SEPARATOR + std::string(pname) + ".obj", nullptr);

        fullcfg.printer_technology.setInt(ptSLA); // FIXME this should be ensured
        fullcfg.set("sla_archive_format", archname);
        fullcfg.set("supports_enable", false);
        fullcfg.set("pad_enable", false);

        DynamicPrintConfig cfg;
        cfg.apply(fullcfg);

        print.set_status_callback([](const PrintBase::SlicingStatus&) {});
        print.apply(m, cfg);
        print.process();

        ThumbnailsList thumbnails;
        auto outputfname = std::string("output_") + pname + "." + SLAArchiveWriter::get_extension(archname);

        print.export_print(outputfname, thumbnails, pname);

        // Not much can be checked about the archives...
        REQUIRE(boost::filesystem::exists(outputfname));

        double vol_written = m.mesh().volume();

        auto readable_formats = SLAArchiveReader::registered_archives();
        if (std::any_of(readable_formats.begin(), readable_formats.end(),
                [&archname](const std::string &a) { return a == archname; })) {

            INFO(std::string("Testing archive type: ") + archname + " -- reading back...");

            indexed_triangle_set its;
            DynamicPrintConfig cfg;

            try {
                // Leave format_id deliberetaly empty, guessing should always
                // work here.
                import_sla_archive(outputfname, "", its, cfg);
            } catch (...) {
                REQUIRE(false);
            }

            // its_write_obj(its, (outputfname + ".obj").c_str());

            REQUIRE(!cfg.empty());
            REQUIRE(!its.empty());

            double vol_read = its_volume(its);
            double rel_err  = std::abs(vol_written - vol_read) / vol_written;
            REQUIRE(rel_err < 0.1);
        }
    }
}
