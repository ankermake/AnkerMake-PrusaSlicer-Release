#include "OrientJob.hpp"
#include "libslic3r/Orientation/AutoOrient.hpp"
#include "libslic3r/Model.hpp"
#include "slic3r/GUI/Plater.hpp"
#include "slic3r/GUI/GLCanvas3D.hpp"

using namespace Slic3r::GUI;
using namespace Slic3r::Orientation;

OrientJob::OrientJob(Plater* plater) :
    m_plater(plater)
{
    
}


void OrientJob::preprocess()
{
    ANKER_LOG_INFO << "OrientJob::preprocess.";
    if (m_plater == nullptr) {
        ANKER_LOG_ERROR << "OrientJob::preprocess m_plater is null.";
        return;
    }
    if (m_isBegin) {
        return;
    }
    m_isBegin = true;
    m_orientCtrl.clearMeshes();
    m_orientCtrl.m_haveSelected = false;
    Selection& _selection = m_plater->get_current_canvas3D()->get_selection();
    Model& model = m_plater->model();
    auto _content = _selection.get_content();
    if (!_content.empty()) {
        m_orientCtrl.m_haveSelected = true;
    }
    auto objects = model.objects;
    for (size_t i = 0; i < objects.size(); i++) {
        auto modelObject = objects[i];
        if (modelObject) {
            for (size_t j = 0; j < modelObject->instances.size(); j++) {
                appendOrientMesh(modelObject, _selection, i, j);
            }
        }
    }
    m_orientCtrl.prepare();
}

void OrientJob::appendOrientMesh(const ModelObject* modelObject, const Selection& _selection, size_t meshIndex, size_t instanceIndex)
{
    if (modelObject == nullptr) {
        ANKER_LOG_ERROR << "OrientJob::appendOrientMesh modelObject is null.";
        return;
    }
    if (instanceIndex < modelObject->instances.size() && instanceIndex < modelObject->volumes.size()) {
        if (m_orientCtrl.m_haveSelected && !(_selection.get_volume(meshIndex)->selected)) {
            return;
        }
        auto instances = modelObject->instances[instanceIndex];
        auto volumes = modelObject->volumes[instanceIndex];
        TriangleMesh mesh = volumes->mesh();
        const Transform3d& transform = instances->get_matrix() * volumes->get_matrix();
        mesh.transform(transform);
        ANKER_LOG_INFO << "name: " << modelObject->name << ", meshIndex: " << meshIndex << ", instanceIndex: " << instanceIndex <<
            "\n" << instances->get_matrix().matrix() << "\n\n" << volumes->get_matrix().matrix() << "\n\n" << transform.matrix() << std::endl;
        AutoOrientMeshPtr meshptr = MAKE_PTR<AutoOrientMesh>(mesh);
        m_orientCtrl.appendOrientMesh(meshptr);
        meshptr->m_index = meshIndex;
        meshptr->m_instanceIndex = instanceIndex;
        if (_selection.get_volume(meshIndex)) {
            meshptr->setMeshSelectedStatus(_selection.get_volume(meshIndex)->selected);
        }
        meshptr->setMeshPrintableStatus(instances->printable);
    }
}

void OrientJob::process(Ctl& ctl)
{
    preprocess();
    ANKER_LOG_INFO << "OrientJob::process.";
    static const auto orientstr = _u8L("Orienting");
    m_orientCtrl.process(ctl, orientstr);
    
    if (m_plater) {
        Model& model = m_plater->model();
        m_orientCtrl.transform(&model);
    }
    ctl.update_status(100, orientstr);
}

void OrientJob::finalize(bool canceled, std::exception_ptr& eptr)
{
    ANKER_LOG_INFO << "OrientJob::finalize.";
    if (m_plater) {
        m_plater->update();
    }
    wxGetApp().aobj_manipul()->set_dirty();
    m_orientCtrl.clearMeshes();
    m_isBegin = false;
    Job::finalize(canceled, eptr);
}
