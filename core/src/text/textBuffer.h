#pragma once

#include "glm/vec2.hpp"
#include "util/typedMesh.h"

struct BufferVert {
    glm::vec2 pos;
    glm::vec2 uv;
    struct State {
        glm::vec2 screenPos;
        float alpha;
        float rotation;
    } state;
};
