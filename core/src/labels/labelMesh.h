#pragma once

#include "gl/vboMesh.h"
#include "labels/label.h"
#include "alfons/atlas.h"

#include <memory>
#include <vector>

namespace Tangram {

namespace alf = alfons;

static constexpr size_t maxLabelMeshVertices = 16384;

class ShaderProgram;

struct GlyphQuad {
    struct {
        glm::i16vec2 pos;
        glm::u16vec2 uv;
    } quad[4];
    // TODO color and stroke must not be stored per quad
    uint32_t color;
    uint32_t stroke;
    alf::AtlasID atlas;
};

class LabelMesh : public VboMesh<Label::Vertex> {

public:

    LabelMesh(std::shared_ptr<VertexLayout> _vertexLayout, GLenum _drawMode);

    virtual ~LabelMesh() override;

    virtual void draw(ShaderProgram& _shader) override;

    void compile(std::vector<Label::Vertex>& _vertices);

    static void loadQuadIndices();

    void pushQuad(GlyphQuad& _quad, Label::Vertex::State& _state);

    void myUpload();

    void clear();

private:

    std::vector<Label::Vertex> m_vertices;
    
    int bufferCapacity = 0;
};

}
