#pragma once

#include "labels/label.h"
#include "style/labelProperty.h"

namespace Tangram {

class SpriteLabel : public Label {
public:

    SpriteLabel(Label::Transform _transform, glm::vec2 _size, LabelMesh& _mesh, int _vertexOffset,
                Label::Options _options, float _extrudeScale, LabelProperty::Anchor _anchor, size_t _hash);

    void updateBBoxes(float _zoomFract) override;
    void align(glm::vec2& _screenPosition, const glm::vec2& _ap1, const glm::vec2& _ap2) override;


private:

    float m_extrudeScale;
    glm::vec2 m_anchor;
};

}
