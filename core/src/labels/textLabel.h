#pragma once

#include "labels/label.h"

#include <glm/glm.hpp>

namespace Tangram {

class TextLabels;
class TextStyle;

struct GlyphQuad {
    size_t atlas;
    struct {
        glm::i16vec2 pos;
        glm::u16vec2 uv;
    } quad[4];
};

using TextRange = std::array<Range, 3>;

struct TextVertex {
    glm::i16vec2 pos;
    glm::u16vec2 uv;
    struct State {
        uint32_t selection;
        uint32_t color;
        uint32_t stroke;
        uint16_t alpha;
        uint16_t scale;
    } state;

    const static float position_scale;
    const static float position_inv_scale;
    const static float alpha_scale;
};

class TextLabel : public Label {

public:

    struct VertexAttributes {
        uint32_t fill;
        uint32_t stroke;
        uint8_t fontScale;
        uint32_t selectionColor;
    };

    using Coordinates = std::array<glm::vec2, 2>;

    TextLabel(Coordinates _coordinates, Type _type, Label::Options _options,
              TextLabel::VertexAttributes _attrib,
              glm::vec2 _dim, TextLabels& _labels, TextRange _textRanges,
              TextLabelProperty::Align _preferedAlignment);

    LabelType renderType() const override { return LabelType::text; }

    bool updateScreenTransform(const glm::mat4& _mvp, const ViewState& _viewState,
                               const AABB* _bounds, ScreenTransform& _transform) override;

    void obbs(ScreenTransform& _transform, OBBBuffer& _obbs) override;

    void addVerticesToMesh(ScreenTransform& _transform, const glm::vec2& _screenSize) override;

    void applyAnchor(LabelProperty::Anchor _anchor) override;

    TextRange& textRanges() {
        return m_textRanges;
    }

    uint32_t selectionColor() override {
        return m_fontAttrib.selectionColor;
    }

    glm::vec2 modelCenter() const override {
        if (m_type == Label::Type::line) {
            return (m_coordinates[0] + m_coordinates[1]) * 0.5f;
        } else {
            return m_coordinates[0];
        }
    }

    float candidatePriority() const override;

protected:

    const Coordinates m_coordinates;

    // Back-pointer to owning container
    const TextLabels& m_textLabels;

    // first vertex and count in m_textLabels quads (left,right,center)
    TextRange m_textRanges;

    // TextRange currently used for drawing
    int m_textRangeIndex;

    VertexAttributes m_fontAttrib;

    // The text LAbel prefered alignment
    TextLabelProperty::Align m_preferedAlignment;
};

}
