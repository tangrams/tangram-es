#pragma once

#include "gl.h"
#include "gl/typedMesh.h"
#include "glm/vec4.hpp"
#include "glm/vec2.hpp"

#include "glfontstash.h" // for fsuint

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

    /* creates a text buffer and bind it */
    void init();

    /* ask the font rasterizer to rasterize a specific text.
     * Returns number of glyphs > 0 on success.
     * @_size is set to the text extents
     * @_bufferOffset is set to the byteOffset of the first glyph-vertex */
    int rasterize(const std::string& _text, glm::vec2& _size, size_t& bufferOffset);

    /* get the vertices from the font context and add them as vbo mesh data */
    void addBufferVerticesToMesh();

private:

    bool m_dirtyTransform;
    fsuint m_fsBuffer;
    int m_bufferPosition;

    std::shared_ptr<FontContext> m_fontContext;

};

}
