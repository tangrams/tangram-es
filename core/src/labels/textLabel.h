#pragma once

#include "labels/label.h"
#include "labels/labelProperty.h"
#include "labels/labelSet.h"
#include "labels/labelMesh.h"
#include "text/fontContext.h"

#include <vector>

namespace Tangram {

class TextLabels;
class TextStyle;

class TextLabel : public Label {

public:

    struct FontVertexAttributes {
        uint32_t fill;
        uint32_t stroke;
        uint8_t fontScale;
    };

    TextLabel(Label::Transform _transform, Type _type, Label::Options _options,
              LabelProperty::Anchor _anchor, TextLabel::FontVertexAttributes _attrib,
              glm::vec2 _dim, TextLabels& _labels, Range _vertexRange);

    void updateBBoxes(float _zoomFract) override;

protected:
    void align(glm::vec2& _screenPosition, const glm::vec2& _ap1, const glm::vec2& _ap2) override;
    glm::vec2 m_anchor;

    void pushTransform() override;

private:
    // Back-pointer to owning container
    TextLabels& m_textLabels;
    // first vertex and count in m_textLabels quads
    Range m_vertexRange;

    FontVertexAttributes m_fontAttrib;
};

struct GlyphQuad {
    size_t atlas;

    struct {
        glm::i16vec2 pos;
        glm::u16vec2 uv;
    } quad[4];
};

class TextLabels : public LabelSet, public StyledMesh {

public:

    TextLabels(const TextStyle& _style) : style(_style) {}

    ~TextLabels() override;

    void draw(ShaderProgram& _shader) override {}
    size_t bufferSize() override { return 0; }

    void setQuads(std::vector<GlyphQuad>& _quads);

    std::vector<GlyphQuad> quads;
    const TextStyle& style;

private:

    std::bitset<maxTextures> m_atlasRefs;
};

}
