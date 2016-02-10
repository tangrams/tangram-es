#pragma once

#include "labels/label.h"
#include "style/labelProperty.h"
#include "labels/labelSet.h"
#include "labels/labelMesh.h"
#include "style/pointStyle.h"

namespace Tangram {

class SpriteLabels;

class SpriteLabel : public Label {
public:

    SpriteLabel(Label::Transform _transform, glm::vec2 _size, Label::Options _options,
                float _extrudeScale, LabelProperty::Anchor _anchor,
                SpriteLabels& _labels, int _vertexOffset);

    void updateBBoxes(float _zoomFract) override;
    void align(glm::vec2& _screenPosition, const glm::vec2& _ap1, const glm::vec2& _ap2) override;

    void pushTransform() override;

private:
    // Back-pointer to owning container
    SpriteLabels& m_spriteLabels;

    float m_extrudeScale;
    glm::vec2 m_anchor;
};

struct SpriteQuad {
    struct {
        glm::i16vec2 pos;
        glm::u16vec2 uv;
        glm::i16vec2 extrude;
    } quad[4];
    // TODO color and stroke must not be stored per quad
    uint32_t color;
};

class SpriteLabels : public LabelSet, public StyledMesh {
public:
    SpriteLabels(const PointStyle& _style) : m_style(_style) {}

    void draw(ShaderProgram& _shader) override {}
    size_t bufferSize() override { return 0; }

    void setQuads(std::vector<SpriteQuad>& _quads) {
        quads.insert(quads.end(),
                     _quads.begin(),
                     _quads.end());
    }

    // TODO: hide within class if needed
    const PointStyle& m_style;
    std::vector<SpriteQuad> quads;
};

}
