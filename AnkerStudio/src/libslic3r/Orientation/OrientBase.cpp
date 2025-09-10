#include "OrientBase.hpp"
#include <ctime>
using namespace Slic3r::Orientation;
const double epsilon = 1e-4;

OrientMeshBase::OrientMeshBase(OrientMeshType meshType, std::function<void(unsigned)> _progress) :
    m_orientMeshType(meshType),
    m_progress(_progress)
{
    m_id = generateId();
}

OrientMeshBase::OrientMeshBase(OrientMeshType meshType, const TriangleMesh& mesh, std::function<void(unsigned)> _progress) :
    m_orientMeshType(meshType),
    m_mesh(mesh),
    m_progress(_progress)
{
    m_id = generateId();
}

int64_t OrientMeshBase::generateId()
{
    return std::time(nullptr);
}

void OrientMeshBase::setMeshSelectedStatus(bool selected)
{
    m_selectedStatus = selected;
}

bool OrientMeshBase::getMeshSelectedStatus() const
{
    return m_selectedStatus;
}

void OrientMeshBase::setMeshPrintableStatus(bool printable)
{
    m_printableStatus = printable;
}

bool OrientMeshBase::getMeshPrintableStatus() const
{
    return m_printableStatus;
}

int64_t OrientMeshBase::getOrientMeshId() const
{
    return m_id;
}

OrientMeshType OrientMeshBase::getOrientMeshType() const
{
    return m_orientMeshType;
}

OrientCtrl::OrientCtrl()
{
}

OrientCtrl::OrientCtrl(const OrientMeshPtrs& orientMeshes) : m_orientMeshPtrs(orientMeshes)
{

}

void OrientCtrl::setOrientMeshes(const OrientMeshPtrs& orientMeshes)
{
    m_orientMeshPtrs = orientMeshes;
}

void OrientCtrl::appendOrientMesh(const OrientMeshBasePtr& orientMesh)
{
    if (orientMesh == nullptr) {
        ANKER_LOG_ERROR << "OrientBase::appendOrientMesh orientMesh is null.";
        return;
    }
    m_orientMeshPtrs.push_back(orientMesh);
}



size_t OrientCtrl::getOrientMeshSize() const
{
    return m_orientMeshPtrs.size();
}


void OrientCtrl::prepare()
{
    for (int i = 0; i < m_orientMeshPtrs.size(); i++) {
        if (m_orientMeshPtrs[i]) {
            m_orientMeshPtrs[i]->prepare();
            ANKER_LOG_INFO << "OrientCtrl::prepare: " << i;
        }
    }
}

void OrientCtrl::process(Slic3r::GUI::Job::Ctl& ctrl, const std::string& str)
{
    for (int i = 0; i < m_orientMeshPtrs.size(); i++) {
        if (m_orientMeshPtrs[i]) {
            ctrl.update_status(((float)(i + 1) / m_orientMeshPtrs.size()) * 100, str);
            m_orientMeshPtrs[i]->process();
            ANKER_LOG_INFO << "OrientCtrl::process: " << i;
        }
    }
}

void OrientCtrl::transform(Slic3r::Model* model)
{
    if (model == nullptr) {
        ANKER_LOG_INFO << "OrientCtrl::finalize model is null.";
        return;
    }
    for (int i = 0; i < m_orientMeshPtrs.size(); i++) {
        if (m_orientMeshPtrs[i] && m_orientMeshPtrs[i]->m_need_orient) {
            int index = m_orientMeshPtrs[i]->m_index;
            int instanceIndex = m_orientMeshPtrs[i]->m_instanceIndex;
            if (index >= 0 && index < model->objects.size()) {
                Matrix3d rot = m_orientMeshPtrs[i]->getRotMatrix();
                auto modelObject = model->objects[index];
                if (modelObject != nullptr) {
                    if (instanceIndex >= 0 && instanceIndex < modelObject->instances.size()) {
                        auto instance = modelObject->instances[instanceIndex];
                        if (instance != nullptr) {
                            //instance->set_rotation(eulerAngles);
                            instance->rotate(rot);
                            double Z = modelObject->instance_bounding_box(instanceIndex).min[2];
                            Vec3d offset = instance->get_offset();
                            offset[2] += -Z;
                            instance->set_offset(offset);
                        }
                    }
                    modelObject->invalidate_bounding_box();
                }
                
            }
        }
    }
    
}

void OrientCtrl::clearMeshes()
{
    m_orientMeshPtrs.clear();
}

