#pragma once

#include "gl.h"
#include "glfontstash.h"
#include "gl/typedMesh.h"
#include "glm/vec4.hpp"
#include "glm/vec2.hpp"

#include <memory>

namespace Tangram {

struct BufferVert {
    glm::vec2 pos;
    glm::vec2 uv;
    struct State {
        glm::vec2 screenPos;
        float alpha;
        float rotation;
    } state;
};

class FontContext;

/*
 * This class represents a text buffer, each text buffer has several text ids
 */
class TextBuffer : public TypedMesh<BufferVert> {

public:

    TextBuffer(std::shared_ptr<VertexLayout> _vertexLayout);
    ~TextBuffer();

    /* generates a text id */
    fsuint genTextID();

    /* creates a text buffer and bind it */
    void init();

    /* ask the font rasterizer to rasterize a specific text for a text id */
    int rasterize(const std::string& _text, fsuint _id, size_t& bufferOffset);

    int getVerticesSize();

    /* get the axis aligned bounding box for a text */
    glm::vec4 getBBox(fsuint _textID);

    /* get the vertices from the font context and add them as vbo mesh data */
    void addBufferVerticesToMesh();

private:

    void bind();
    void unbind();

    bool m_bound;
    bool m_dirtyTransform;
    fsuint m_fsBuffer;
    int m_bufferPosition;

    std::shared_ptr<FontContext> m_fontContext;

};

}
