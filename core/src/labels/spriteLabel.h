#pragma once

#include "labels/label.h"
#include "labels/labelSet.h"

#include <array>

namespace Tangram {

class SpriteLabels;
class PointStyle;
class Texture;

struct SpriteVertex {
    glm::vec3 pos;
    glm::u16vec2 uv;
    struct State {
        uint32_t selection;
        uint32_t color;
        uint16_t alpha;
        uint16_t scale;
    } state;

    static const float alpha_scale;
    static const float texture_scale;
};

class SpriteLabel : public Label {
public:

    struct VertexAttributes {
        uint32_t color;
        uint32_t selectionColor;
        float extrudeScale;
    };

    SpriteLabel(Label::WorldTransform _transform, glm::vec2 _size, Label::Options _options,
                SpriteLabel::VertexAttributes _attrib, Texture* _texture,
                SpriteLabels& _labels, size_t _labelsPos);

    void updateBBoxes(float _zoomFract) override;

    void addVerticesToMesh() override;

private:

    void applyAnchor(LabelProperty::Anchor _anchor) override;

    bool updateScreenTransform(const glm::mat4& _mvp, const ViewState& _viewState, bool _drawAllLabels) override;

    // Back-pointer to owning container and position
    const SpriteLabels& m_labels;
    const size_t m_labelsPos;

    // Non-owning reference to a texture that is specific to this label.
    // If non-null, this indicates a custom texture for a marker.
    Texture* m_texture;

    VertexAttributes m_vertexAttrib;

    std::array<glm::vec3, 4> m_projected;
};

struct SpriteQuad {
    struct {
        glm::vec2 pos;
        glm::u16vec2 uv;
    } quad[4];
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
