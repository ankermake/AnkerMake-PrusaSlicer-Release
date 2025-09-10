#ifndef slic3r_OrientBase_hpp_
#define slic3r_OrientBase_hpp_

#include "slic3r/GUI/Jobs/Job.hpp"
#include "libslic3r/Model.hpp"
#include "OrientTriangleMesh.hpp"
#include "Eigen/Dense"

#define DEF_PTR(className) class className; typedef std::shared_ptr<className> className##Ptr;

namespace Slic3r {

namespace Orientation {

	template<typename T, typename... Args>
	std::shared_ptr<T> MAKE_PTR(Args&&... args) {
		return std::make_shared<T>(std::forward<Args>(args)...);
	}

	enum OrientMeshType : unsigned int{
		OrientMeshType_Base,
		OrientMeshType_AutoOrient,
	};

	DEF_PTR(OrientMeshBase)
	class OrientMeshBase {
	public:
		OrientMeshBase(OrientMeshType meshType, std::function<void(unsigned)> _progress = nullptr);
		OrientMeshBase(OrientMeshType meshType, const TriangleMesh& mesh, std::function<void(unsigned)> _progress = nullptr);
		virtual ~OrientMeshBase() = default;
		void setMeshSelectedStatus(bool selected);
		bool getMeshSelectedStatus() const;
		int64_t getOrientMeshId() const;
		OrientMeshType getOrientMeshType()const;

		void setMeshPrintableStatus(bool printable);
		bool getMeshPrintableStatus() const;

		std::pair<Vec3d, double> getRotAngle() const{   return m_rot_angle; };
		Matrix3d getRotMatrix() const { return m_rot; }

		std::function<void(unsigned)> m_progress = nullptr;

		int m_index = -1; // Select modelobjects index.
		int m_instanceIndex = -1; // Select instances index.
		bool m_need_orient = false;

		virtual void prepare() = 0;
		virtual void process() = 0;
		virtual void finalize() = 0;



	private:
		int64_t generateId();
		
	protected:
		int64_t m_id = 0;  
		std::string m_name = "";
		OrientMeshType m_orientMeshType = OrientMeshType_Base;
		bool m_selectedStatus = false;
		bool m_printableStatus = false;
		bool m_parallel = false; // tbb::parallel_for
		std::pair<Vec3d, double> m_rot_angle; // eg: ((0,0,1), 30),  it represents a rotation of 30 degrees about the Z axis
		Matrix3d m_rot;
		OrientTriangleMesh m_mesh;
	};
	typedef std::vector<OrientMeshBasePtr> OrientMeshPtrs;

	DEF_PTR(OrientCtrl)
	class OrientCtrl
	{
	public:
		OrientCtrl();
		OrientCtrl(const OrientMeshPtrs& orientMeshes);
		void setOrientMeshes(const OrientMeshPtrs& orientMeshes);
		void appendOrientMesh(const OrientMeshBasePtr& orientMesh);
		
		void clearMeshes();
		void prepare();
		void process(Slic3r::GUI::Job::Ctl & ctrl, const std::string &str);
		void transform(Slic3r::Model* model = nullptr);

		bool m_haveSelected = false;

	private:
		size_t getOrientMeshSize() const;

	private:
		OrientMeshPtrs m_orientMeshPtrs; // all meshed.
	};
	
}

}


#endif // slic3r_OrientBase_hpp_
