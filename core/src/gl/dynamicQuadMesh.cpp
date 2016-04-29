#include "gl/dynamicQuadMesh.h"
#include "gl/renderState.h"
#include "gl/shaderProgram.h"

namespace Tangram {

GLuint QuadIndices::quadIndexBuffer = 0;
int QuadIndices::quadGeneration = -1;
std::atomic<int> QuadIndices::meshCounter(0);


void QuadIndices::ref() {
    meshCounter++;
}

void QuadIndices::unref() {
    meshCounter--;

    if (quadIndexBuffer != 0 && (!RenderState::isValidGeneration(quadGeneration) ||
                                 meshCounter <= 0)) {

        if (RenderState::indexBuffer.compare(quadIndexBuffer)) {
            RenderState::indexBuffer.init(0, false);
        }
        GL_CHECK(glDeleteBuffers(1, &quadIndexBuffer));
        quadIndexBuffer = 0;
        quadGeneration = -1;
    }
}

void QuadIndices::load() {
    if (RenderState::isValidGeneration(quadGeneration)) {

        RenderState::indexBuffer(quadIndexBuffer);
        return;
    }

    quadGeneration = RenderState::generation();

    std::vector<GLushort> indices;
    indices.reserve(maxVertices / 4 * 6);

    for (size_t i = 0; i < maxVertices; i += 4) {
        indices.push_back(i + 2);
        indices.push_back(i + 0);
        indices.push_back(i + 1);
        indices.push_back(i + 1);
        indices.push_back(i + 3);
        indices.push_back(i + 2);
    }

    GL_CHECK(glGenBuffers(1, &quadIndexBuffer));
    RenderState::indexBuffer(quadIndexBuffer);
    GL_CHECK(glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLushort),
                 reinterpret_cast<GLbyte*>(indices.data()), GL_STATIC_DRAW));
}

}
