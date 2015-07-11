#pragma once

#include "gl.h"
#include "glfontstash.h"
#include "util/typedMesh.h"
#include "style/style.h"

#include "glm/vec2.hpp"

#include "style/style.h" // for Batch base class

#include <memory>

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
class TextStyle;

/*
 * This class represents a text buffer, each text buffer has several text ids
 */
class TextBatch : public Batch {

public:

    TextBatch(const TextStyle& _style);
    
    virtual void draw(const View& _view) override;
    virtual void update(float _dt, const View& _view) override {};
    virtual bool compile() {
        if (m_mesh->numVertices() > 0) {
            m_mesh->compileVertexBuffer();
            return true;
        }
        return false;
    };
    
    ~TextBatch();
    
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
    void transformID(fsuint _textID, const BufferVert::State& _state);

    void pushBuffer();

    int getVerticesSize();

    /* get the axis aligned bounding box for a text */
    glm::vec4 getBBox(fsuint _textID);

    /* get the vertices from the font context and add them as vbo mesh data */
    void addBufferVerticesToMesh();

//private:

    void bind();
    void unbind();

    bool m_bound;
    bool m_dirtyTransform;
    fsuint m_fsBuffer;
    std::shared_ptr<FontContext> m_fontContext;

    std::shared_ptr<TypedMesh<BufferVert>> m_mesh;

    const TextStyle& m_style;
};
