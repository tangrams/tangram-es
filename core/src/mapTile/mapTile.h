/*
...
*/
#pragma once

#include "glm/glm.hpp"
#include <vector>

class MapTile {


    //vertex data for vbo
    std::vector<float> m_Vertices; //in glBufferData, use &m_vertices.front()
    //std::vector<int> m_Indices; //in glBufferData, use &m_indices.front()

  public:
    glm::ivec3 m_MercXYZ;
    MapTile(glm::ivec3 _mercXYZ);
    //Helper Functions
    bool setVBO(std::vector<float> _vboData);
    glm::vec2 GetBoundingBox();
    ~MapTile() {
        m_Vertices.clear();
    }
};
