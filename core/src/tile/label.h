#pragma once

#include "glfontstash.h"
#include "text/fontContext.h"
#include "glm/glm.hpp"
#include <string>

struct Label {
    // std::shared_ptr<FontContext> m_fontContext;
    fsuint m_id;
    std::string m_text;
    glm::dvec2 m_worldPosition;
    float m_alpha;
    float m_rotation;
};
