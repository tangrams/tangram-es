#include "label.h"

Label::Label(LabelTransform _transform, std::string _text, std::shared_ptr<TextBuffer> _buffer) :
    m_transform(_transform),
    m_text(_text),
    m_buffer(_buffer) {

    m_id = m_buffer->genTextID();
}

Label::~Label() {

}

void Label::rasterize() {
    m_buffer->rasterize(m_text, m_id);
}

void Label::updateTransform(LabelTransform _transform, glm::dmat4 _mvp, glm::dvec2 _screenSize) {
    m_transform = _transform;

    float halfWidth = _screenSize.x * 0.5;
    float halfHeight = _screenSize.y * 0.5;

    float alpha = m_transform.m_alpha;

    glm::dvec4 screenPosition = glm::dvec4(m_transform.m_worldPosition, 0.0, 1.0);

    // mimic gpu vertex projection to screen
    screenPosition = _mvp * screenPosition;
    screenPosition = screenPosition / screenPosition.w; // perspective division

    // from normalized device coordinates to screen space coordinate system
    // top-left screen axis, y pointing down
    screenPosition.x = (screenPosition.x + 1) * halfWidth;
    screenPosition.y = (1 - screenPosition.y) * halfHeight;

    // don't display out of screen labels, and out of screen translations or not yet implemented in fstash
    alpha = screenPosition.x > _screenSize.x || screenPosition.x < 0 ? 0.0 : m_transform.m_alpha;
    alpha = screenPosition.y > _screenSize.y || screenPosition.y < 0 ? 0.0 : m_transform.m_alpha;

    m_buffer->transformID(m_id, screenPosition.x, screenPosition.y, m_transform.m_rotation, alpha);
}
