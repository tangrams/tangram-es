#pragma once

#include "labels/label.h"
#include "labels/labelSet.h"

#include <array>

namespace Tangram {

class SpriteLabels;
class PointStyle;

struct SpriteVertex {
    glm::vec4 pos;
    glm::u16vec2 uv;
    struct State {
        uint32_t color;
        uint16_t alpha;
        uint16_t scale;
    } state;

    static const float position_scale;
    static const float alpha_scale;
    static const float texture_scale;
};

class SpriteLabel : public Label {
public:

    SpriteLabel(Label::WorldTransform _transform, glm::vec2 _size, Label::Options _options,
                float _extrudeScale, SpriteLabels& _labels, size_t _labelsPos);

    void updateBBoxes(float _zoomFract) override;

    void addVerticesToMesh() override;

private:

    void applyAnchor(LabelProperty::Anchor _anchor) override;

    bool updateScreenTransform(const glm::mat4& _mvp, const ViewState& _viewState, bool _drawAllLabels) override;

    // Back-pointer to owning container and position
    const SpriteLabels& m_labels;
    const size_t m_labelsPos;

    float m_extrudeScale;

    std::array<glm::vec4, 4> m_projected;

    glm::vec2 m_viewportSize;
};

struct SpriteQuad {
    struct {
        glm::i16vec2 pos;
        glm::u16vec2 uv;
    } quad[4];
    // TODO color and stroke must not be stored per quad
    uint32_t color;
};

class SpriteLabels : public LabelSet {
public:
    SpriteLabels(const PointStyle& _style) : m_style(_style) {}

    void setQuads(std::vector<SpriteQuad>&& _quads) {
        quads = std::move(_quads);
    }

    // TODO: hide within class if needed
    const PointStyle& m_style;
    std::vector<SpriteQuad> quads;
};

}
