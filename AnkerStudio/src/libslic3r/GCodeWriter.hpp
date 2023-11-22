#ifndef slic3r_GCodeWriter_hpp_
#define slic3r_GCodeWriter_hpp_

#include "libslic3r.h"
#include <string>
#include <charconv>
#include "Extruder.hpp"
#include "Point.hpp"
#include "PrintConfig.hpp"
#include "GCode/CoolingBuffer.hpp"

namespace Slic3r {

class GCodeWriter {
public:
    GCodeConfig config;
    bool multiple_extruders;
    
    GCodeWriter() : 
        multiple_extruders(false), m_extrusion_axis("E"), m_extruder(nullptr),
        m_single_extruder_multi_material(false),
        m_last_acceleration(0), m_max_acceleration(0),
        m_last_bed_temperature(0), m_last_bed_temperature_reached(true), 
        m_lifted(0)
        {}
    Extruder*            extruder()             { return m_extruder; }
    const Extruder*      extruder()     const   { return m_extruder; }

    // Returns empty string for gcfNoExtrusion.
    std::string          extrusion_axis() const { return m_extrusion_axis; }
    void                 apply_print_config(const PrintConfig &print_config);
    // Extruders are expected to be sorted in an increasing order.
    void                 set_extruders(std::vector<unsigned int> extruder_ids);
    const std::vector<Extruder>& extruders() const { return m_extruders; }
    std::vector<unsigned int> extruder_ids() const { 
        std::vector<unsigned int> out; 
        out.reserve(m_extruders.size()); 
        for (const Extruder &e : m_extruders) 
            out.push_back(e.id()); 
        return out;
    }
    std::string preamble();
    std::string postamble() const;
    std::string set_temperature(unsigned int temperature, bool wait = false, int tool = -1) const;
    std::string set_bed_temperature(unsigned int temperature, bool wait = false);
    std::string set_print_acceleration(unsigned int acceleration)   { return set_acceleration_internal(Acceleration::Print, acceleration); }
    std::string set_travel_acceleration(unsigned int acceleration)  { return set_acceleration_internal(Acceleration::Travel, acceleration); }
    std::string reset_e(bool force = false);
    std::string update_progress(unsigned int num, unsigned int tot, bool allow_100 = false) const;
    // return false if this extruder was already selected
    bool        need_toolchange(unsigned int extruder_id) const 
        { return m_extruder == nullptr || m_extruder->id() != extruder_id; }
    std::string set_extruder(unsigned int extruder_id)
        { return this->need_toolchange(extruder_id) ? this->toolchange(extruder_id) : ""; }
    // Prefix of the toolchange G-code line, to be used by the CoolingBuffer to separate sections of the G-code
    // printed with the same extruder.
    std::string toolchange_prefix() const;
    std::string toolchange(unsigned int extruder_id);
    std::string set_speed(double F, const std::string &comment = std::string(), const std::string &cooling_marker = std::string()) const;
    std::string travel_to_xy(const Vec2d &point, const std::string &comment = std::string());
    std::string travel_to_xyz(const Vec3d &point, const std::string &comment = std::string());
    std::string travel_to_z(double z, const std::string &comment = std::string());
    bool        will_move_z(double z) const;
    std::string extrude_to_xy(const Vec2d &point, double dE, const std::string &comment = std::string());
//    std::string extrude_to_xyz(const Vec3d &point, double dE, const std::string &comment = std::string());
    std::string retract(bool before_wipe = false);
    std::string retract_for_toolchange(bool before_wipe = false);
    std::string unretract();
    std::string lift();
    std::string unlift();

    // Current position of the printer, in G-code coordinates.
    // Z coordinate of current position contains zhop. If zhop is applied (this->zhop() > 0),
    // then the print_z = this->get_position().z() - this->zhop().
    Vec3d       get_position() const { return m_pos; }
    // Current Z hop value.
    double      get_zhop() const { return m_lifted; }
    // Update position of the print head based on the final position returned by a custom G-code block.
    // The new position Z coordinate contains the Z-hop.
    // GCodeWriter expects the custom script to NOT change print_z, only Z-hop, thus the print_z is maintained
    // by this function while the current Z-hop accumulator is updated.
    void        update_position(const Vec3d &new_pos);

    // Returns whether this flavor supports separate print and travel acceleration.
    static bool supports_separate_travel_acceleration(GCodeFlavor flavor);

    // To be called by the CoolingBuffer from another thread.
    static std::string set_fan(const GCodeFlavor gcode_flavor, bool gcode_comments, unsigned int speed);
    // To be called by the main thread. It always emits the G-code, it does not remember the previous state.
    // Keeping the state is left to the CoolingBuffer, which runs asynchronously on another thread.
    std::string set_fan(unsigned int speed) const;

private:
	// Extruders are sorted by their ID, so that binary search is possible.
    std::vector<Extruder> m_extruders;
    std::string     m_extrusion_axis;
    bool            m_single_extruder_multi_material;
    Extruder*       m_extruder;
    unsigned int    m_last_acceleration = (unsigned int)(-1);
    unsigned int    m_last_travel_acceleration = (unsigned int)(-1); // only used for flavors supporting separate print/travel acc
    // Limit for setting the acceleration, to respect the machine limits set for the Marlin firmware.
    // If set to zero, the limit is not in action.
    unsigned int    m_max_acceleration;
    unsigned int    m_max_travel_acceleration;

    unsigned int    m_last_bed_temperature;
    bool            m_last_bed_temperature_reached;
    double          m_lifted;
    Vec3d           m_pos = Vec3d::Zero();

    enum class Acceleration {
        Travel,
        Print
    };

    std::string _travel_to_z(double z, const std::string &comment);
    std::string _retract(double length, double restart_extra, const std::string &comment);
    std::string set_acceleration_internal(Acceleration type, unsigned int acceleration);
};

class GCodeFormatter {
public:
    GCodeFormatter() {
        this->buf_end = buf + buflen;
        this->ptr_err.ptr = this->buf;
    }

    GCodeFormatter(const GCodeFormatter&) = delete;
    GCodeFormatter& operator=(const GCodeFormatter&) = delete;

    // At layer height 0.15mm, extrusion width 0.2mm and filament diameter 1.75mm,
    // the crossection of extrusion is 0.4 * 0.15 = 0.06mm2
    // and the filament crossection is 1.75^2 = 3.063mm2
    // thus the filament moves 3.063 / 0.6 = 51x slower than the XY axes
    // and we need roughly two decimal digits more on extruder than on XY.
#if 1
    static constexpr const int XYZF_EXPORT_DIGITS = 3;
    static constexpr const int E_EXPORT_DIGITS    = 5;
#else
    // order of magnitude smaller extrusion rate erros
    static constexpr const int XYZF_EXPORT_DIGITS = 4;
    static constexpr const int E_EXPORT_DIGITS    = 6;
    // excessive accuracy
//    static constexpr const int XYZF_EXPORT_DIGITS = 6;
//    static constexpr const int E_EXPORT_DIGITS    = 9;
#endif

    static constexpr const std::array<double, 10> pow_10    {   1.,     10.,    100.,    1000.,    10000.,    100000.,    1000000.,    10000000.,    100000000.,    1000000000.};
    static constexpr const std::array<double, 10> pow_10_inv{1./1.,  1./10., 1./100., 1./1000., 1./10000., 1./100000., 1./1000000., 1./10000000., 1./100000000., 1./1000000000.};

    // Quantize doubles to a resolution of the G-code.
    static double                                 quantize(double v, size_t ndigits) { return std::round(v * pow_10[ndigits]) * pow_10_inv[ndigits]; }
    static double                                 quantize_xyzf(double v) { return quantize(v, XYZF_EXPORT_DIGITS); }
    static double                                 quantize_e(double v) { return quantize(v, E_EXPORT_DIGITS); }

    void emit_axis(const char axis, const double v, size_t digits);

    void emit_xy(const Vec2d &point) {
        this->emit_axis('X', point.x(), XYZF_EXPORT_DIGITS);
        this->emit_axis('Y', point.y(), XYZF_EXPORT_DIGITS);
    }

    void emit_xyz(const Vec3d &point) {
        this->emit_axis('X', point.x(), XYZF_EXPORT_DIGITS);
        this->emit_axis('Y', point.y(), XYZF_EXPORT_DIGITS);
        this->emit_z(point.z());
    }

    void emit_z(const double z) {
        this->emit_axis('Z', z, XYZF_EXPORT_DIGITS);
    }

    void emit_e(const std::string &axis, double v) {
        if (! axis.empty()) {
            // not gcfNoExtrusion
            this->emit_axis(axis[0], v, E_EXPORT_DIGITS);
        }
    }

    void emit_f(double speed) {
        this->emit_axis('F', speed, XYZF_EXPORT_DIGITS);
    }

    void emit_string(const std::string &s) {
        strncpy(ptr_err.ptr, s.c_str(), s.size());
        ptr_err.ptr += s.size();
    }

    void emit_comment(bool allow_comments, const std::string &comment) {
        if (allow_comments && ! comment.empty()) {
            *ptr_err.ptr ++ = ' '; *ptr_err.ptr ++ = ';'; *ptr_err.ptr ++ = ' ';
            this->emit_string(comment);
        }
    }

    std::string string() {
        *ptr_err.ptr ++ = '\n';
        return std::string(this->buf, ptr_err.ptr - buf);
    }

protected:
    static constexpr const size_t   buflen = 256;
    char                            buf[buflen];
    char* buf_end;
    std::to_chars_result            ptr_err;
};

class GCodeG1Formatter : public GCodeFormatter {
public:
    GCodeG1Formatter() {
        this->buf[0] = 'G';
        this->buf[1] = '1';
        this->buf_end = buf + buflen;
        this->ptr_err.ptr = this->buf + 2;
    }

    GCodeG1Formatter(const GCodeG1Formatter&) = delete;
    GCodeG1Formatter& operator=(const GCodeG1Formatter&) = delete;
};

} /* namespace Slic3r */

#endif /* slic3r_GCodeWriter_hpp_ */
