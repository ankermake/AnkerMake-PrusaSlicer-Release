#ifndef slic3r_OrientTriangleMesh_hpp
#define slic3r_OrientTriangleMesh_hpp

#include "libslic3r/Geometry.hpp"
#include "libslic3r/TriangleMesh.hpp"

namespace Slic3r {

namespace Orientation {

    struct Vector3fHash {
        size_t operator() (const Vec3f& k) const {
            size_t h1 = std::hash<float>()(k[0]);
            size_t h2 = std::hash<float>()(k[1]);
            size_t h3 = std::hash<float>()(k[2]);
            return h1 ^ (h2 << 1) ^ (h3 << 2);
        }
    };
    class OrientTriangleMesh : public TriangleMesh {
    public:
        OrientTriangleMesh() {};
        OrientTriangleMesh(const TriangleMesh& mesh) : TriangleMesh(mesh) {};

        const Eigen::VectorXf& getAreas() const { return m_areas; };
        const Eigen::MatrixXf& getNormals() const { return m_normals; }
        const Eigen::VectorXf& getIsApperances() const { return m_is_apperance; }

        void computeMeshBaseProperty();
        void computeMeshApperance();
        void computeQuantizedNormalVectorArea(std::vector<Eigen::Vector3f>& maxAreaDirections, int maxDirectionNum = 10);
        static bool areaPointsInSamePlane(const std::vector<Eigen::Vector3f>& points);
        static bool arePointsContinuous(const std::vector<Eigen::Vector3f>& points);
        static bool areContinuous(const Eigen::Vector3f& t1, const Eigen::Vector3f& t2, const Eigen::Vector3f& t3);

        void computeProjectVertices(const Vec3f& orientation);

        //void computeBottomAreas(const Vec3f& orientation, const std::vector<std::pair<Vec3f, float>>& areas);

        Eigen::MatrixXf m_z_projected;
        Eigen::VectorXf m_z_max, m_z_median, m_z_mean;

    private:
        int m_face_count = 0;
        Eigen::MatrixXf m_normals, m_normals_quantize;
        Eigen::VectorXf m_areas, m_is_apperance;
        std::vector<Vec3f> m_face_normals; // calc all face nomals.
    };


}

}

#endif


