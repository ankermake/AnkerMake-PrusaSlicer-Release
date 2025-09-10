#ifndef slic3r_AutoOrient_hpp_
#define slic3r_AutoOrient_hpp_ 

#include "OrientBase.hpp"

namespace Slic3r {

namespace Orientation {

    struct AutoOrientParams {
        float m_overhang_angle = 60.0;
        float first_lay_height = 0.14f;
        float apperance_face_supp = 3.0;
        float ascent = -0.5f;

        float overhang_threshold = 1.0f;
        float bottom_threshold = 1.0f;

        float laf_max = 0.999f;
        float laf_min = 0.9703f;

        float overhang_relative = 0.05;// 0.24308070476924726f;
        float lowAngleArea_relative = 0;// 0.01f;
        float bottom_relative = 0.35;// 1.167152017941474f;
        float contour_relative = 0.15;// 0.23228623269775997f;
        float bottom_hull_relative = 0.2;// 0.1f;
        float contour_hull_relative = 0.05;
    };


    struct CostParams {
        float overhang = 0.0;
        float bottomAreas = 0.0;
        float lowAngleAreas = 0.0;
        float contour = 0.0;
        float bottomHullAreas = 0.0;
        float contourHull = 0.0;
        float resultCost = 0;
    };

    DEF_PTR(AutoOrientMesh)
    class AutoOrientMesh : public OrientMeshBase {
    public:
        AutoOrientMesh(const AutoOrientParams& params = AutoOrientParams()) :
            OrientMeshBase(OrientMeshType_AutoOrient),
            m_params(params)
            {};
        AutoOrientMesh(const TriangleMesh& mesh, const AutoOrientParams& params = AutoOrientParams()) : 
            OrientMeshBase(OrientMeshType_AutoOrient, mesh),
            m_params(params)
            {};

        void prepare() override;
        void process() override;
        void finalize() override;

        std::function<void(unsigned)> m_processind = {};

        void computeRotationFromTwoVectors(const Vec3d& from, const Vec3d& to, Matrix3d& rotation_matrix);
    private:
        void computeMeshBaseProperty();
        void computeMeshApperance();
        void computeMeshBaseConvexHull();
        void computeQuantizedNormalVectorArea();

        void addSupplements();

        void removeDuplicates();

        void computeOverhang(const Vec3f& orientation, CostParams& cost);
        float computeCost(const CostParams& cost);

        void minMaxNormalize(CostParams& cost);
        
    private:
        OrientTriangleMesh m_mesh_convex_hull;
        AutoOrientParams m_params;
        std::vector<Eigen::Vector3f> m_max_area_directions;

        bool m_need_cost_normalize = false;
    };

}

}


#endif // slic3r_AutoOrient_hpp_
