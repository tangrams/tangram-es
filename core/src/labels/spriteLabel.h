#pragma once

#include "labels/label.h"

namespace Tangram {

class SpriteLabel : public Label {
public:

    SpriteLabel(Label::Transform _transform, glm::vec2 _size, LabelMesh& _mesh, int _vertexOffset,
                Label::Options _options, float _extrudeScale, glm::vec2 _anchor = glm::vec2(0.5f));

    void updateBBoxes(float _zoomFract) override;
    void align(glm::vec2& _screenPosition, const glm::vec2& _ap1, const glm::vec2& _ap2) override;

private:
    glm::vec2 m_anchor;

private:

    float m_extrudeScale;
};

}
