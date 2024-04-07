#include "OrientTriangleMesh.hpp"
#include "libslic3r/SurfaceMesh.hpp"
#include "libslic3r/Utils.hpp"
#include "Eigen/Dense"
#include <cmath>
#include <unordered_map>

using namespace Slic3r::Orientation;       
using namespace Eigen;

const double epsilon = 0.001;

static  Vector3f quantize_vec3f(const Vector3f& n1) {
    return Vector3f(floor(n1(0) * 1000) / 1000, floor(n1(1) * 1000) / 1000, floor(n1(2) * 1000) / 1000);
}


void OrientTriangleMesh::computeMeshBaseProperty()
{
    m_face_count = facets_count();
    m_face_normals = its_face_normals(its);
    m_areas = VectorXf::Zero(m_face_count);      

    m_is_apperance = VectorXf::Zero(m_face_count);
    m_normals = MatrixXf::Zero(m_face_count, 3);
    m_normals_quantize = MatrixXf::Zero(m_face_count, 3);

    for (size_t i = 0; i < m_face_count; i++) {
        m_normals.row(i) =  m_face_normals[i]; 
        m_normals_quantize.row(i) = quantize_vec3f(m_normals.row(i));
        m_areas(i) = its.facet_area(i);
    }
}

void OrientTriangleMesh::computeMeshApperance()
{
   // OrientSurfaceMesh surfaceMesh(its, m_face_normals);
    //m_is_apperance = surfaceMesh.get_face_is_out_side();
}

void OrientTriangleMesh::computeQuantizedNormalVectorArea(std::vector<Eigen::Vector3f>& maxAreaDirections, int maxDirectionNum)
{
    struct AreaProperty {
        float currentArea = 0.0;
        float totArea = 0.0;
        size_t count = 0;
        Vector3f normal;
        std::vector<size_t> facet_idxs;
        size_t first_face_idx = -1;
        AreaProperty() {};
        AreaProperty(float curr, float total, const Vector3f& nor) :
            currentArea(curr),
            totArea(total),
            normal(nor) {};
    };

    std::unordered_map<Vector3f, AreaProperty, Vector3fHash> unorderedMap;
    Vector3f n1;
    float currAreas = 0.0;
    float totalAreas = 0.0;
    for (size_t i = 0; i < m_areas.size(); i++) {
        unorderedMap.insert(std::pair(m_normals_quantize.row(i), AreaProperty(currAreas, totalAreas, n1)));
    }
    for (size_t i = 0; i < m_areas.size(); i++) {
        if (/*m_is_apperance(i)*/1) {
            unorderedMap[m_normals_quantize.row(i)].totArea += m_areas[i];
            if (m_areas[i] > unorderedMap[m_normals_quantize.row(i)].currentArea) {
                unorderedMap[m_normals_quantize.row(i)].normal = m_normals.row(i);
                unorderedMap[m_normals_quantize.row(i)].currentArea = m_areas[i];
            }
        }
    }

    typedef std::pair<Vector3f, AreaProperty> QuantizePair;
    std::vector<QuantizePair> sortVec(unorderedMap.begin(), unorderedMap.end());
    std::sort(sortVec.begin(), sortVec.end(), [](const QuantizePair& q1, const QuantizePair& q2){   return q1.second.totArea > q2.second.totArea; });
    maxDirectionNum = std::min((size_t)maxDirectionNum, sortVec.size());
    for (size_t i = 0; i < maxDirectionNum; i++) {
        //std::vector<Vector3f> points;
        //for (size_t j = 0; j < sortVec[i].second.facet_idxs.size(); j++) {
        //    points.push_back(its.get_vertex(sortVec[i].second.facet_idxs[j], 0));
        //    points.push_back(its.get_vertex(sortVec[i].second.facet_idxs[j], 1));
        //    points.push_back(its.get_vertex(sortVec[i].second.facet_idxs[j], 2));
        //}
        //
        maxAreaDirections.push_back(sortVec[i].second.normal);
        //if (areaPointsInSamePlane(points)) {
        //   
        //}
    }
}

bool OrientTriangleMesh::areaPointsInSamePlane(const std::vector<Eigen::Vector3f>& points)
{
    if (points.size() < 9) {
        return false;
    }
    Eigen::Vector3f v1 = points[1] - points[0];
    Eigen::Vector3f v2 = points[2] - points[0];
    Eigen::Vector3f normal = v1.cross(v2).normalized();

    for (size_t i = 3; i < points.size(); i+=3) {
        Eigen::Vector3f v1 = points[i + 1] - points[i];
        Eigen::Vector3f v2 = points[i + 2] - points[i];
        Eigen::Vector3f currentNormal = v1.cross(v2).normalized();

        if (std::abs(normal.dot(currentNormal)) < 1.0f - epsilon)
            return false;

        float distance = std::abs((points[i] - points[0]).dot(normal));
        if (distance > epsilon)
            return false;
    }
    return true;
}

bool OrientTriangleMesh::arePointsContinuous(const std::vector<Eigen::Vector3f>& points)
{
    if (points.size() % 3 != 0) {
        return false;
    }

    for (size_t i = 0; i < points.size() - 3; i+=3) {
        if (!areContinuous(points[i + 1], points[i + 2], points[i + 3])) {
            return false;
         }
    }

    return true;
}

bool OrientTriangleMesh::areContinuous(const Eigen::Vector3f& t1, const Eigen::Vector3f& t2, const Eigen::Vector3f& t3)
{
    if ((t1 == t2) || (t1 == t3)) {
        return true;
    }
    return false;
}


void OrientTriangleMesh::computeProjectVertices(const Vec3f& orientation)
{
    float norm_squared = orientation.squaredNorm();
    int faceCount = facets_count();
    m_z_projected.resize(faceCount, 3);
    m_z_max.resize(faceCount, 1);
    m_z_median.resize(faceCount, 1);
    m_z_mean.resize(faceCount, 1);
    for (size_t i = 0; i < faceCount; i++) {
        stl_vertex vertex0 = its.get_vertex(i, 0);
        stl_vertex vertex1 = its.get_vertex(i, 1);
        stl_vertex vertex2 = its.get_vertex(i, 2);
        float z0 = vertex0.dot(orientation);
        float z1 = vertex1.dot(orientation);
        float z2 = vertex2.dot(orientation); 
        m_z_projected(i, 0) = z0;
        m_z_projected(i, 1) = z1;
        m_z_projected(i, 2) = z2;
        m_z_max(i) = std::max(std::max(z0, z1), z2);
        m_z_median(i) = std::max(std::min(z0, z1), std::min(std::max(z0, z1), z2));
        m_z_mean(i) = (z0 + z1 + z2) / 3;
    }
}


