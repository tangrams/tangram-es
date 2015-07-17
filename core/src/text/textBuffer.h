#pragma once

#include "gl.h"
#include "glfontstash.h"
#include "gl/typedMesh.h"
#include "glm/vec4.hpp"
#include "glm/vec2.hpp"

#include <memory>

struct BufferVert {
    glm::vec2 pos;
    glm::vec2 uv;
    glm::vec2 screenPos;
    float alpha;
    float rotation;
};

class FontContext;

/*
 * This class represents a text buffer, each text buffer has several text ids
 */
class TextBuffer : public TypedMesh<BufferVert> {

public:

    TextBuffer(std::shared_ptr<FontContext> _fontContext, std::shared_ptr<VertexLayout> _vertexLayout);
    ~TextBuffer();

    /* generates a text id */
    fsuint genTextID();

    /* creates a text buffer and bind it */
    void init();

    /* ask the font rasterizer to rasterize a specific text for a text id */
    bool rasterize(const std::string& _text, fsuint _id);

    /*
     * transform a text id in screen space coordinate
     *  x, y in screen space
     *  rotation is in radians
     *  alpha should be in [0..1]
     */
    void transformID(fsuint _textID, float _x, float _y, float _rot, float _alpha);

    void pushBuffer();

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
    std::shared_ptr<FontContext> m_fontContext;

};
