#pragma once

#include "fontstash/glfontstash.h"
#include "glm/glm.hpp"
#include <string>

struct Label {
    FONScontext* m_fontContext;
    fsuint m_id;
    std::string m_text;
    glm::dvec2 m_worldPosition;
    float m_alpha;
    float m_rotation;
};