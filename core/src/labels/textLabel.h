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

struct TextRange {
    TextLabelProperty::Align align;
    Range range;
};

struct TextVertex {
    glm::i16vec2 pos;
    glm::u16vec2 uv;
    struct State {
        uint32_t color;
        uint32_t stroke;
        uint8_t alpha;
        uint8_t scale;
        int16_t rotation;
    } state;

    const static float position_scale;
    const static float rotation_scale;
    const static float alpha_scale;
};

class TextLabel : public Label {

public:

    struct FontVertexAttributes {
        uint32_t fill;
        uint32_t stroke;
        uint8_t fontScale;
    };

    TextLabel(Label::Transform _transform, Type _type, Label::Options _options,
              LabelProperty::Anchor _anchor, TextLabel::FontVertexAttributes _attrib,
              glm::vec2 _dim, TextLabels& _labels, std::vector<TextRange> _textRanges,
              TextLabelProperty::Align _preferedAlignment);

    void updateBBoxes(float _zoomFract) override;

    std::vector<TextRange>& textRanges() {
        return m_textRanges;
    }

protected:

    void pushTransform() override;

private:

    void applyAnchor(const glm::vec2& _dimension, const glm::vec2& _origin,
                     LabelProperty::Anchor _anchor) override;

    // Back-pointer to owning container
    const TextLabels& m_textLabels;

    // first vertex and count in m_textLabels quads
    std::vector<TextRange> m_textRanges;

    // TextRange currently used for drawing
    int m_textRangeIndex;

    FontVertexAttributes m_fontAttrib;

    // The text LAbel prefered alignment
    TextLabelProperty::Align m_preferedAlignment;

};

}
