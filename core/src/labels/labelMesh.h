#pragma once

#include "gl/vboMesh.h"
#include "labels/label.h"

#include <memory>
#include <vector>

namespace Tangram {

class ShaderProgram;

struct SpriteQuad;
struct GlyphQuad;

class LabelMesh : public VboMesh<Label::Vertex> {

public:

    LabelMesh(std::shared_ptr<VertexLayout> _vertexLayout, GLenum _drawMode);

    virtual ~LabelMesh() override;

    void draw(ShaderProgram& _shader, bool _clear = true) override;

    void pushQuad(const GlyphQuad& _quad, const Label::Vertex::State& _state);
    void pushQuad(const SpriteQuad& _quad, const Label::Vertex::State& _state);

    void myUpload();

    bool isReady() { return m_isUploaded; }
private:

    static void loadQuadIndices();

    std::vector<Label::Vertex> m_vertices;
};

}
