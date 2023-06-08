#ifndef slic3r_FillEnsuring_hpp_
#define slic3r_FillEnsuring_hpp_

#include "FillBase.hpp"
#include "FillRectilinear.hpp"

namespace Slic3r {

class FillEnsuring : public FillRectilinear
{
public:
    Fill *clone() const override { return new FillEnsuring(*this); }
    ~FillEnsuring() override = default;
    Polylines      fill_surface(const Surface *surface, const FillParams &params) override { return {}; };
    ThickPolylines fill_surface_arachne(const Surface *surface, const FillParams &params) override;

protected:
    void fill_surface_single_arachne(const Surface &surface, const FillParams &params, ThickPolylines &thick_polylines_out);

    bool no_sort() const override { return true; }

    // PrintRegionConfig is used for computing overlap between boundary contour and inner Rectilinear infill.
    const PrintRegionConfig *print_region_config = nullptr;

    friend class Layer;
};

} // namespace Slic3r

#endif // slic3r_FillEnsuring_hpp_
