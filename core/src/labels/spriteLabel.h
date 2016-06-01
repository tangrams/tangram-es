#pragma once

#include "labels/label.h"
#include "labels/labelSet.h"

namespace Tangram {

class SpriteLabels;
class PointStyle;

struct SpriteVertex {
    glm::i16vec2 pos;
    glm::u16vec2 uv;
    uint32_t color;
    glm::i16vec2 extrude;
    struct State {
        glm::i16vec2 screenPos;
        uint8_t alpha;
        uint8_t scale;
        int16_t rotation;
    } state;

    static const float position_scale;
    static const float rotation_scale;
    static const float alpha_scale;
    static const float texture_scale;
    static const float extrusion_scale;
};

class SpriteLabel : public Label {
public:

    SpriteLabel(Label::Transform _transform, glm::vec2 _size, Label::Options _options,
                float _extrudeScale, LabelProperty::Anchor _anchor,
                SpriteLabels& _labels, size_t _labelsPos);

    void updateBBoxes(float _zoomFract) override;

    void pushTransform() override;

private:

    void applyAnchor(const glm::vec2& _dimension, const glm::vec2& _origin,
        LabelProperty::Anchor _anchor) override;
    
    // Back-pointer to owning container and position
    const SpriteLabels& m_labels;
    const size_t m_labelsPos;

    float m_extrudeScale;
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

class SpriteLabels : public LabelSet {
public:
    SpriteLabels(const PointStyle& _style) : m_style(_style) {}

    void setQuads(std::vector<SpriteQuad>& _quads) {
        quads.insert(quads.end(), _quads.begin(), _quads.end());
    }

    // TODO: hide within class if needed
    const PointStyle& m_style;
    std::vector<SpriteQuad> quads;
};

}
