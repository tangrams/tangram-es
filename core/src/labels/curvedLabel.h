#pragma once

#include "labels/label.h"
#include "labels/textLabel.h"

#include <glm/glm.hpp>

namespace Tangram {

class TextLabels;
class TextStyle;

class CurvedLabel : public TextLabel {

public:

    struct VertexAttributes {
        uint32_t fill;
        uint32_t stroke;
        uint8_t fontScale;
        uint32_t selectionColor;
    };

    using ModelTransform = std::vector<glm::vec2>;

    CurvedLabel(ModelTransform _modelTransform, Label::Options _options, float _prio,
                TextLabel::VertexAttributes _attrib, glm::vec2 _dim,
                TextLabels& _labels, TextRange _textRanges,
                TextLabelProperty::Align _preferedAlignment, size_t _anchorPoint)

        : TextLabel({{}}, Label::Type::curved, _options, _attrib,
                    _dim, _labels, _textRanges, _preferedAlignment),
          m_modelTransform(std::move(_modelTransform)),
          m_anchorPoint(_anchorPoint),
          m_screenAnchorPoint(_anchorPoint),
          m_prio(_prio) {

        applyAnchor(m_options.anchors[0]);
    }

    bool updateScreenTransform(const glm::mat4& _mvp, const ViewState& _viewState,
                               const AABB* _bounds, ScreenTransform& _transform) override;

    void obbs(ScreenTransform& _transform, OBBBuffer& _obbs) override;

    void addVerticesToMesh(ScreenTransform& _transform, const glm::vec2& _screenSize) override;

    void applyAnchor(LabelProperty::Anchor _anchor) override;

    float candidatePriority() const override {
        return m_prio;
    }

    glm::vec2 modelCenter() const override {
        return m_modelTransform[m_anchorPoint];
    }

protected:

    const std::vector<glm::vec2> m_modelTransform;

    const size_t m_anchorPoint;

    size_t m_screenAnchorPoint;

    float m_prio = 0;
};

}
