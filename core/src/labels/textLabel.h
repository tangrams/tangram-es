#pragma once

#include "labels/label.h"
#include "labels/labelMesh.h"
#include "text/fontContext.h"
#include "style/labelProperty.h"

namespace Tangram {

class TextLabel : public Label {

public:
    TextLabel(Label::Transform _transform, Type _type, glm::vec2 _dim, LabelMesh& _mesh, Range _vertexRange,
              Label::Options _options, FontContext::FontMetrics _metrics, int _nLines, LabelProperty::Anchor _anchor,
              glm::vec2 _quadsLocalOrigin);

    void updateBBoxes(float _zoomFract) override;

protected:

    void align(glm::vec2& _screenPosition, const glm::vec2& _ap1, const glm::vec2& _ap2) override;
    FontContext::FontMetrics m_metrics;
    int m_nLines;

    void pushTransform() override;

private:
    // Back-pointer to owning container
    LabelMesh& m_mesh;

    glm::vec2 m_anchor;
    glm::vec2 m_quadLocalOrigin;

};

}
