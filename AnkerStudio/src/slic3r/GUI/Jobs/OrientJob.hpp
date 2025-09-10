#ifndef slic3r_OrientJob_hpp_
#define slic3r_OrientJob_hpp_

#include "libslic3r/Orientation/OrientBase.hpp"
#include "Job.hpp"
#include "slic3r/GUI/Selection.hpp"

namespace Slic3r {
    namespace GUI {
        class Plater;
        class OrientJob : public Job {
        public:
            OrientJob(Plater* plater);
            void process(Ctl& ctl) override;
            void finalize(bool canceled, std::exception_ptr& eptr) override;
            void preprocess();

        private:
            void appendOrientMesh(const ModelObject* modelObject, const Selection& _selection, size_t meshIndex, size_t instanceIndex);

        private:
            Orientation::OrientCtrl m_orientCtrl;
            Plater* m_plater = nullptr;
            bool m_isBegin = false;
        };

    }
}


#endif // !slic3r_OrientJob_hpp_
