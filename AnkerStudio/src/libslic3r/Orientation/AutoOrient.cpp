#include "AutoOrient.hpp"
#include "libslic3r/Geometry.hpp"
using namespace Slic3r::Orientation;
const double tol = 0.000001;
const double epsilon = 1e-4;

void AutoOrientMesh::prepare()
{
    m_max_area_directions.clear();
    computeMeshBaseProperty();
    computeMeshApperance();
    computeMeshBaseConvexHull();
}

void AutoOrientMesh::process()
{
    computeQuantizedNormalVectorArea();
    ANKER_LOG_INFO << "computeQuantizedNormalVectorArea finished.";
    //addSupplements();

    if (m_progress) {
        m_progress(30);
    }

   // removeDuplicates();

    std::vector<std::pair<Vec3f, CostParams>> costs;
    for (int i = 0; i < m_max_area_directions.size(); i++) {
        auto orientation = -m_max_area_directions[i];
        m_mesh.computeProjectVertices(orientation);
        m_mesh_convex_hull.computeProjectVertices(orientation);
        CostParams cost;
        computeOverhang(orientation, cost);

        if (abs(cost.bottomAreas) < epsilon) {
            continue;
        }
        cost.resultCost = computeCost(cost);
        //std::cout << "orientation: (" << -orientation.x() << ", " << -orientation.y() << ", " << -orientation.z() << "), cost: " << result << std::endl;
        costs.push_back(std::pair<Vec3f, CostParams>(-orientation, cost));
    }
    ANKER_LOG_INFO << "computeCost finished.";
    std::sort(costs.begin(), costs.end(), [](const std::pair<Vec3f, CostParams>& p1, const std::pair<Vec3f, CostParams>& p2) {
        return p1.second.resultCost < p2.second.resultCost; });
    if (costs.empty()) {
        ANKER_LOG_INFO << "m_max_area_directions is empty.";
        return;
    }  
    ANKER_LOG_INFO << "std::sort finished.";

    Eigen::Vector3f orientation = costs[0].first;
    Eigen::Vector3f zAxis = { 0, 0, 1 };
    bool is_z_axis_direction = false;
    std::vector<std::pair<Vec3f, CostParams>> sameCosts;
    for (size_t i = 1; i < costs.size(); i++) {
        if (abs(costs[i].second.bottomAreas - costs[0].second.bottomAreas) < epsilon) {
            if (abs(costs[0].first.dot(zAxis) - 1 > epsilon)) {
                if (abs(costs[i].first.dot(zAxis) - 1) < epsilon * epsilon) {
                    orientation = zAxis;
                    is_z_axis_direction = true;
                    break;
                }
            }
            else {
                sameCosts.push_back(costs[i]);
            }
        }
    }

    if (!is_z_axis_direction && !sameCosts.empty()) {
        // Gets the set of directions with the smallest acos<n0,n1> values.
        Eigen::Vector3f n0 = -zAxis;
        float theta = acosf(n0.dot(costs[0].first)) / (n0.norm() * costs[0].first.norm());
        for (size_t i = 0; i < sameCosts.size(); i++) {
            float tmpTheta = acosf(n0.dot(sameCosts[i].first)) / (n0.norm() * sameCosts[i].first.norm());
            if (theta > tmpTheta) {
                theta = tmpTheta;
                orientation = sameCosts[i].first;
            }
        }
    }

    m_need_orient = !orientation.cast<double>().isApprox(Vec3d(0, 0, -1));
    if (m_need_orient) {
        computeRotationFromTwoVectors(orientation.cast<double>(), Vec3d(0, 0, -1), m_rot);
    }
    ANKER_LOG_INFO << "orientation: (" << orientation.x() << ", " << orientation.y() << ", " << orientation.z() << "), cost: " << costs[0].second.resultCost;
}

void AutoOrientMesh::finalize()
{
}

void AutoOrientMesh::computeMeshBaseProperty()
{
    m_mesh.computeMeshBaseProperty();
}

void AutoOrientMesh::computeMeshApperance()
{
    m_mesh.computeMeshApperance();
}

void AutoOrientMesh::computeMeshBaseConvexHull()
{
    m_mesh_convex_hull = m_mesh.convex_hull_3d();
    m_mesh_convex_hull.computeMeshBaseProperty();
}

void AutoOrientMesh::computeQuantizedNormalVectorArea()
{
     m_mesh.computeQuantizedNormalVectorArea(m_max_area_directions, 30);
     m_mesh_convex_hull.computeQuantizedNormalVectorArea(m_max_area_directions, 30);
}


void AutoOrientMesh::addSupplements()
{
    std::vector<Vec3f> ballVec = { {0, 0, -1} ,{0.70710678, 0, -0.70710678},{0, 0.70710678, -0.70710678},
            {-0.70710678, 0, -0.70710678},{0, -0.70710678, -0.70710678},
            {1, 0, 0},{0.70710678, 0.70710678, 0},{0, 1, 0},{-0.70710678, 0.70710678, 0},
            {-1, 0, 0},{-0.70710678, -0.70710678, 0},{0, -1, 0},{0.70710678, -0.70710678, 0},
            {0.70710678, 0, 0.70710678},{0, 0.70710678, 0.70710678},
            {-0.70710678, 0, 0.70710678},{0, -0.70710678, 0.70710678},{0, 0, 1} };
    m_max_area_directions.insert(m_max_area_directions.end(), ballVec.begin(), ballVec.end());
}

void AutoOrientMesh::removeDuplicates()
{
    for (auto it = m_max_area_directions.begin() + 1; it < m_max_area_directions.end(); )
    {
        bool duplicate = false; 
        for (auto it_ok = m_max_area_directions.begin(); it_ok < it; it_ok++)
        {
            if (it_ok->isApprox(*it, tol)) {
                duplicate = true;
                break;
            }
        }
        const Vec3f all_zero = { 0,0,0 };
        if (duplicate || it->isApprox(all_zero, tol))
            it = m_max_area_directions.erase(it);
        else
            it++;
    }
}


void AutoOrientMesh::computeOverhang(const Vec3f& orientation, CostParams& cost)
{
    //double boxArea = m_mesh.bounding_box().area();
    //double radius = m_mesh.bounding_box().radius();
    //double volume = m_mesh.stats().volume > 0 ? m_mesh.stats().volume : its_volume(m_mesh.its);

    float total_min_z = m_mesh.m_z_projected.minCoeff();
    float hull_total_min_z = m_mesh_convex_hull.m_z_projected.minCoeff();
    auto bottom_condition = m_mesh.m_z_max.array() < total_min_z + m_params.first_lay_height + epsilon;
    auto bottom_condition_hull = m_mesh_convex_hull.m_z_max.array() < hull_total_min_z + m_params.first_lay_height + epsilon;
    auto bottom_condition_2nd = m_mesh.m_z_max.array() < total_min_z + m_params.first_lay_height / 2.0 + epsilon;

    const Eigen::VectorXf& allAreas = m_mesh.getAreas();
    const Eigen::VectorXf& hullAllAreas = m_mesh_convex_hull.getAreas();
    cost.bottomAreas = bottom_condition.select(allAreas, 0).sum();// *0.5 + bottom_condition_2nd.select(allAreas, 0).sum();
    cost.bottomHullAreas = bottom_condition_hull.select(hullAllAreas, 0).sum();
   const Eigen::MatrixXf& normals = m_mesh.getNormals();
    Eigen::VectorXf normalProj(normals.rows(), 1);
    for (size_t i = 0; i < normals.rows(); i++) {
        normalProj(i) = normals.row(i).dot(orientation);
    }

    const Eigen::VectorXf& isApperances = m_mesh.getIsApperances();
   
    auto appearanceAreas = allAreas.cwiseProduct((isApperances * m_params.apperance_face_supp) + Eigen::VectorXf::Ones(isApperances.rows(), isApperances.cols()));
    auto overhangAreas = ((normalProj.array() < m_params.ascent) * (!bottom_condition_2nd)).select(appearanceAreas, 0);

    cost.overhang = overhangAreas.array().cwiseAbs().sum();
    cost.contour = 4 * sqrt(cost.bottomAreas);
    cost.contourHull = 4 * sqrt(cost.bottomHullAreas);

    Eigen::MatrixXf normalProjAbs = normalProj.cwiseAbs();
    ANKER_LOG_INFO << "orientation: (" << -orientation.x() << "," << -orientation.y() << "," << -orientation.z()
        << "), bottomArea:  " << cost.bottomAreas << ", hull bottomArea: " << cost.bottomHullAreas
        << ", overhang: " << cost.overhang << ", contour: " << cost.contour
        << ", hull contour: " << cost.contourHull << ", lowAngleAreas: " << cost.lowAngleAreas;

    m_need_cost_normalize = false;
    if (cost.overhang / cost.bottomAreas > 1.0f &&
        cost.overhang > m_params.overhang_threshold &&
        cost.bottomAreas > m_params.bottom_threshold) {
        m_need_cost_normalize = true;
        minMaxNormalize(cost);
    }
       
}


float AutoOrientMesh::computeCost(const CostParams& cost)
{
    // When overhang approaches 0, the maximum bottom is used as the basis for calculation.
    float numerator = m_params.overhang_relative * (cost.overhang);
    float denominator = m_params.bottom_relative * cost.bottomAreas +
        m_params.bottom_hull_relative * cost.bottomHullAreas +
        m_params.contour_relative * cost.contour +
        m_params.contour_hull_relative * cost.contourHull;
    if (!m_need_cost_normalize && cost.overhang < m_params.overhang_threshold) {
        numerator = 1.0f;
    }
    if (!m_need_cost_normalize && cost.bottomAreas < m_params.bottom_threshold) {
        denominator = 1.0f;
    }
    return numerator / denominator;
}

void AutoOrientMesh::minMaxNormalize(CostParams& cost)
{
    std::vector<float> data;
    data.push_back(cost.bottomAreas);
    data.push_back(cost.bottomHullAreas);
    data.push_back(cost.contour);
    data.push_back(cost.contourHull);
    data.push_back(cost.overhang);
    float minVal = *std::min_element(data.begin(), data.end());
    float maxVal = *std::max_element(data.begin(), data.end());
    cost.bottomAreas = (cost.bottomAreas - minVal) / (maxVal - minVal);
    cost.bottomHullAreas = (cost.bottomHullAreas - minVal) / (maxVal - minVal);
    cost.contour = (cost.contour - minVal) / (maxVal - minVal);
    cost.contourHull = (cost.contourHull - minVal) / (maxVal - minVal);
    cost.overhang = (cost.overhang - minVal) / (maxVal - minVal);
}

void AutoOrientMesh::computeRotationFromTwoVectors(const Vec3d& from, const Vec3d& to, Matrix3d& rotation_matrix)
{
    double epsilon = 1e-3;
    Vec3d rotation_axis = { 0.0, 0.0, 0.0 };
    double phi = 0;
    if ((from + to).isMuchSmallerThan(1, epsilon)) // The two vectors are almost opposite.
    {
        rotation_axis << 1, 0, 0;
        phi = M_PI;
    }
    else if ((from - to).isMuchSmallerThan(1, epsilon)) {  // Both vectors are the same.
        rotation_axis << 1, 0, 0;
        phi = 0;
    }
    else {
        rotation_axis = from.cross(to);
        rotation_axis.normalize();
        double dot_product = from.dot(to);
        dot_product = std::max(-1.0, std::min(dot_product, 1.0)); // clamp dot_product to [-1,1].
        phi = acos(dot_product);
    }

    // Use quaternion to calculate rotation matrix
    double half_phi = phi / 2.0;
    double sin_half_phi = sin(half_phi);
    rotation_axis[0] = abs(rotation_axis[0]) < epsilon ? 0 : rotation_axis[0];
    rotation_axis[1] = abs(rotation_axis[1]) < epsilon ? 0 : rotation_axis[1];
    rotation_axis[2] = abs(rotation_axis[2]) < epsilon ? 0 : rotation_axis[2];
    std::cout << "rotation_axis: " << rotation_axis[0] << ", " << rotation_axis[1] << ", " << rotation_axis[2] << std::endl;
    Eigen::Quaterniond q(cos(half_phi), rotation_axis[0] * sin_half_phi, rotation_axis[1] * sin_half_phi, rotation_axis[2] * sin_half_phi);
    rotation_matrix = q.toRotationMatrix();
}

