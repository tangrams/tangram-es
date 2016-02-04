#pragma once

#include "labels/labelMesh.h"
#include "alfons/atlas.h"

namespace Tangram {

namespace alf = alfons;

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

class TextMesh : public LabelMesh {
    using LabelMesh::LabelMesh;

public:
    void pushQuad(GlyphQuad& _quad, Label::Vertex::State& _state);
    void myUpload();
    void clear();

private:
    int bufferCapacity = 0;
    std::vector<Label::Vertex> m_vertices;

};

}
