#include "FillBedJob.hpp"

#include "libslic3r/Model.hpp"
#include "libslic3r/ClipperUtils.hpp"

#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/Plater.hpp"
#include "slic3r/GUI/GLCanvas3D.hpp"
#include "slic3r/GUI/GUI_ObjectList.hpp"

#include <numeric>

namespace Slic3r {
namespace GUI {

void FillBedJob::prepare()
{
    m_selected.clear();
    m_unselected.clear();
    m_bedpts.clear();
    m_min_bed_inset = 0.;

    m_object_idx = m_plater->get_selected_object_idx();
    if (m_object_idx == -1)
        return;

    ModelObject *model_object = m_plater->model().objects[m_object_idx];
    if (model_object->instances.empty()) return;

    m_selected.reserve(model_object->instances.size());
    for (ModelInstance *inst : model_object->instances)
        if (inst->printable) {
            ArrangePolygon ap = get_arrange_poly(inst, m_plater);
            // Existing objects need to be included in the result. Only
            // the needed amount of object will be added, no more.
            ++ap.priority;
            m_selected.emplace_back(ap);
        }

    if (m_selected.empty()) return;

    m_bedpts = get_bed_shape(*m_plater->config());

    auto &objects = m_plater->model().objects;
    BoundingBox bedbb = get_extents(m_bedpts);

    for (size_t idx = 0; idx < objects.size(); ++idx)
        if (int(idx) != m_object_idx)
            for (ModelInstance *mi : objects[idx]->instances) {
                ArrangePolygon ap = get_arrange_poly(PtrWrapper{mi}, m_plater);
                auto ap_bb = ap.transformed_poly().contour.bounding_box();

                if (ap.bed_idx == 0 && !bedbb.contains(ap_bb))
                    ap.bed_idx = arrangement::UNARRANGED;

                m_unselected.emplace_back(ap);
            }

    if (auto wt = get_wipe_tower_arrangepoly(*m_plater))
        m_unselected.emplace_back(std::move(*wt));

    double sc = scaled<double>(1.) * scaled(1.);

    ExPolygon poly = m_selected.front().poly;
    double poly_area = poly.area() / sc;
    double unsel_area = std::accumulate(m_unselected.begin(),
                                        m_unselected.end(), 0.,
                                        [](double s, const auto &ap) {
                                            return s + (ap.bed_idx == 0) * ap.poly.area();
                                        }) / sc;

    double fixed_area = unsel_area + m_selected.size() * poly_area;
    double bed_area   = Polygon{m_bedpts}.area() / sc;

    // This is the maximum number of items, the real number will always be close but less.
    int needed_items = (bed_area - fixed_area) / poly_area;

    int sel_id = m_plater->get_selection().get_instance_idx();
    // if the selection is not a single instance, choose the first as template
    sel_id = std::max(sel_id, 0);
    ModelInstance *mi = model_object->instances[sel_id];
    ArrangePolygon template_ap = get_arrange_poly(PtrWrapper{mi}, m_plater);

    for (int i = 0; i < needed_items; ++i) {
        ArrangePolygon ap = template_ap;
        ap.poly = m_selected.front().poly;
        ap.bed_idx = arrangement::UNARRANGED;
        ap.setter = [this, mi](const ArrangePolygon &p) {
            ModelObject *mo = m_plater->model().objects[m_object_idx];
            ModelInstance *inst = mo->add_instance(*mi);
            inst->apply_arrange_result(p.translation.cast<double>(), p.rotation);
        };
        m_selected.emplace_back(ap);
    }

    m_status_range = m_selected.size();

    // The strides have to be removed from the fixed items. For the
    // arrangeable (selected) items bed_idx is ignored and the
    // translation is irrelevant.
    double stride = bed_stride(m_plater);
    for (auto &p : m_unselected)
        if (p.bed_idx > 0)
            p.translation(X) -= p.bed_idx * stride;

    coord_t min_offset = 0;
    for (auto &ap : m_selected) {
        min_offset = std::max(ap.inflation, min_offset);
    }

    if (m_plater->printer_technology() == ptSLA) {
        // Apply the max offset for all the objects
        for (auto &ap : m_selected) {
            ap.inflation = min_offset;
        }
    } else { // it's fff, brims only need to be minded from bed edges
        for (auto &ap : m_selected) {
            ap.inflation = 0;
        }
        m_min_bed_inset = min_offset;
    }
}

void FillBedJob::process(Ctl &ctl)
{
    auto statustxt = _u8L("Filling bed");
    arrangement::ArrangeParams params;
    ctl.call_on_main_thread([this, &params] {
           prepare();
           params = get_arrange_params(m_plater);
           coord_t min_inset = get_skirt_offset(m_plater) + m_min_bed_inset;
           params.min_bed_distance = std::max(params.min_bed_distance, min_inset);
    }).wait();
    ctl.update_status(0, statustxt);

    if (m_object_idx == -1 || m_selected.empty())
        return;

    bool do_stop = false;
    params.stopcondition = [&ctl, &do_stop]() {
        return ctl.was_canceled() || do_stop;
    };

    params.progressind = [this, &ctl, &statustxt](unsigned st) {
        if (st > 0)
            ctl.update_status(int(m_status_range - st) * 100 / status_range(), statustxt);
    };

    params.on_packed = [&do_stop] (const ArrangePolygon &ap) {
        do_stop = ap.bed_idx > 0 && ap.priority == 0;
    };

    arrangement::arrange(m_selected, m_unselected, m_bedpts, params);

    // finalize just here.
    ctl.update_status(100, ctl.was_canceled() ?
                                      _u8L("Bed filling canceled.") :
                                      _u8L("Bed filling done."));
}

FillBedJob::FillBedJob() : m_plater{wxGetApp().plater()} {}

void FillBedJob::finalize(bool canceled, std::exception_ptr &eptr)
{
    // Ignore the arrange result if aborted.
    if (canceled || eptr)
        return;

    if (m_object_idx == -1) return;

    ModelObject *model_object = m_plater->model().objects[m_object_idx];
    if (model_object->instances.empty()) return;

    size_t inst_cnt = model_object->instances.size();

    int added_cnt = std::accumulate(m_selected.begin(), m_selected.end(), 0, [](int s, auto &ap) {
        return s + int(ap.priority == 0 && ap.bed_idx == 0);
    });

    if (added_cnt > 0) {
        for (ArrangePolygon &ap : m_selected) {
            if (ap.bed_idx != arrangement::UNARRANGED && (ap.priority != 0 || ap.bed_idx == 0))
                ap.apply();
        }

        model_object->ensure_on_bed();

        m_plater->update();

        // FIXME: somebody explain why this is needed for increase_object_instances
        if (inst_cnt == 1) added_cnt++;

        m_plater->sidebar()
            .obj_list()->increase_object_instances(m_object_idx, size_t(added_cnt));
    }
}

}} // namespace Slic3r::GUI
