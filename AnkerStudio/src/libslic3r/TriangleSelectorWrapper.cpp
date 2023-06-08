#include "TriangleSelectorWrapper.hpp"
#include <memory>

namespace Slic3r {

TriangleSelectorWrapper::TriangleSelectorWrapper(const TriangleMesh &mesh, const Transform3d& mesh_transform) :
        mesh(mesh), mesh_transform(mesh_transform), selector(mesh), triangles_tree(
                AABBTreeIndirect::build_aabb_tree_over_indexed_triangle_set(mesh.its.vertices, mesh.its.indices)) {
}

void TriangleSelectorWrapper::enforce_spot(const Vec3f &point, const Vec3f &origin, float radius) {
    std::vector<igl::Hit> hits;
    Vec3f dir = (point - origin).normalized();
    if (AABBTreeIndirect::intersect_ray_all_hits(mesh.its.vertices, mesh.its.indices, triangles_tree,
            Vec3d(origin.cast<double>()),
            Vec3d(dir.cast<double>()),
            hits)) {
        for (int hit_idx = hits.size() - 1; hit_idx >= 0; --hit_idx) {
            const igl::Hit &hit = hits[hit_idx];
            Vec3f pos = origin + dir * hit.t;
            Vec3f face_normal = its_face_normal(mesh.its, hit.id);
            if ((point - pos).norm() < radius && face_normal.dot(dir) < 0) {
                std::unique_ptr<TriangleSelector::Cursor> cursor = std::make_unique<TriangleSelector::Sphere>(
                        pos, origin, radius, this->mesh_transform, TriangleSelector::ClippingPlane { });
                selector.select_patch(hit.id, std::move(cursor), EnforcerBlockerType::ENFORCER, Transform3d::Identity(),
                        true, 0.0f);
                break;
            }
        }
    } else {
        size_t hit_idx_out;
        Vec3f hit_point_out;
        float dist = AABBTreeIndirect::squared_distance_to_indexed_triangle_set(mesh.its.vertices, mesh.its.indices,
                triangles_tree, point, hit_idx_out, hit_point_out);
        if (dist < radius) {
            std::unique_ptr<TriangleSelector::Cursor> cursor = std::make_unique<TriangleSelector::Sphere>(
                    point, origin, radius, this->mesh_transform, TriangleSelector::ClippingPlane { });
            selector.select_patch(hit_idx_out, std::move(cursor), EnforcerBlockerType::ENFORCER,
                    Transform3d::Identity(),
                    true, 0.0f);
        }
    }
}

}
