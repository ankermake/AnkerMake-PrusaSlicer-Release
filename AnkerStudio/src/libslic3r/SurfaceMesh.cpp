#include "SurfaceMesh.hpp"

#include "Eigen/Dense"
using namespace Slic3r;

// Let the model center of gravity orientation approximate the model orientation.
void OrientSurfaceMesh::compute_model_direction()
{
    //m_model_direction = Eigen::Vector3d(0, 0, 0);
    //for (size_t i = 0; i < m_face_normals.size(); i++) {
    //    m_model_direction += m_face_normals[i].cast<double>();
    //}
    //m_model_direction /= m_face_normals.size();
    //m_model_direction.normalize();
}

// Aden
/**
* 
* 
We first initialize a 'visited' array to keep track of the faces we have visited, 
then we start with the first face and mark it as external. Then, we use a stack to go through all the faces. 
For each face, we check all of its adjacent faces, and if the dot product of the normal vector of the 
adjacent face and the normal vector of the current face is greater than 0, then we think they point in the 
same direction, otherwise, we think they point in the opposite direction. 
We then set the 'is_outside' flag of the adjacent surface to be the same or opposite as the current surface, 
depending on whether their normal vectors point in the same direction. Finally, 
we add the adjacency to the stack so that it can be accessed in subsequent iterations.

*/

void OrientSurfaceMesh::compute_face_out_side()
{
    m_is_outside.resize(m_face_normals.size());
    std::vector<bool> visited(m_face_normals.size());
    std::stack<int> s;
    s.push(0);
    m_is_outside[0] = true;

    int count = 0;
    while (!s.empty()) {
        int current = s.top();
        s.pop();
        if (current < visited.size() && current < m_face_neighbors.size()) {
            visited[current] = true;
            for (int neighbor : m_face_neighbors[current]) {
                if (neighbor < visited.size() && !visited[neighbor]) {
                    if (m_face_normals[current].dot(m_face_normals[neighbor]) > 0) {
                        m_is_outside[neighbor] = m_is_outside[current];
                    }
                    else {
                        m_is_outside[neighbor] = !m_is_outside[current];
                    }
                    s.push(neighbor);
                }
            }
        }
    }
}


const Eigen::VectorXf& OrientSurfaceMesh::get_face_is_out_side() const
{
    return m_is_outside;
}