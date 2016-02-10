#pragma once

#include "gl/mesh.h"
#include "labels/label.h"

#include <memory>
#include <vector>

namespace Tangram {

static constexpr size_t maxLabelMeshVertices = 16384;

class ShaderProgram;

struct SpriteQuad;
struct GlyphQuad;

class LabelMesh : public Mesh<Label::Vertex> {

public:

    LabelMesh(std::shared_ptr<VertexLayout> _vertexLayout, GLenum _drawMode);

    virtual ~LabelMesh() override;

    void draw(ShaderProgram& _shader) override;

    void compile(std::vector<Label::Vertex>& _vertices);

    bool compiled() { return m_isCompiled; }

    void clear();

    size_t numberOfVertices() const { return m_vertices.size(); }

    void pushQuad(const GlyphQuad& _quad, const Label::Vertex::State& _state);
    void pushQuad(const SpriteQuad& _quad, const Label::Vertex::State& _state);

    void myUpload();

private:

    static void loadQuadIndices();

    std::vector<Label::Vertex> m_vertices;
};

}
