#pragma once

#include "labels/label.h"
#include "labels/textLabel.h"

#include <glm/glm.hpp>

namespace Tangram {

class TextLabels;
class TextStyle;

class CurvedLabel : public Label {

public:

    struct VertexAttributes {
        uint32_t fill;
        uint32_t stroke;
        uint8_t fontScale;
        uint32_t selectionColor;
    };

    CurvedLabel(Label::Options _options, float _prio,
                TextLabel::VertexAttributes _attrib,
                glm::vec2 _dim, TextLabels& _labels, TextRange _textRanges,
                TextLabelProperty::Align _preferedAlignment,
                size_t _anchorPoint, const std::vector<glm::vec2>& _line);

    LabelType renderType() const override { return LabelType::text; }

    TextRange& textRanges() {
        return m_textRanges;
    }

    void obbs(ScreenTransform& _transform, std::vector<OBB>& _obbs,
              Range& _range, bool _append) override;


    float candidatePriority() const {
        return m_prio;
    }

protected:

    void addVerticesToMesh(ScreenTransform& _transform) override;

    uint32_t selectionColor() override {
        return m_fontAttrib.selectionColor;
    }

private:

    void applyAnchor(LabelProperty::Anchor _anchor) override;

    bool updateScreenTransform(const glm::mat4& _mvp, const ViewState& _viewState,
                               ScreenTransform& _transform, bool _drawAllLabels) override;

    // Back-pointer to owning container
    const TextLabels& m_textLabels;

    // first vertex and count in m_textLabels quads (left,right,center)
    TextRange m_textRanges;

    // TextRange currently used for drawing
    int m_textRangeIndex;

    TextLabel::VertexAttributes m_fontAttrib;

    // The text LAbel prefered alignment
    TextLabelProperty::Align m_preferedAlignment;

    size_t m_anchorPoint;
    std::vector<glm::vec2> m_line;

    size_t m_screenAnchorPoint;

    float m_prio = 0;
};

}
