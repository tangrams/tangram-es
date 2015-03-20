#include "label.h"

Label::Label(LabelTransform _transform, std::string _text, std::shared_ptr<TextBuffer> _buffer) :
    m_transform(_transform),
    m_text(_text),
    m_buffer(_buffer) {

    m_id = m_buffer->genTextID();
}

Label::~Label() {}

void Label::rasterize() {
    m_buffer->rasterize(m_text, m_id);
}

void Label::updateTransform(const LabelTransform& _transform, const glm::mat4& _mvp, const glm::vec2& _screenSize) {
    m_transform = _transform;

    float alpha = m_transform.m_alpha;

    glm::vec2 p1 = worldToScreenSpace(_mvp, glm::vec4(m_transform.m_modelPosition1, 0.0, 1.0), _screenSize);
    glm::vec2 p2 = worldToScreenSpace(_mvp, glm::vec4(m_transform.m_modelPosition2, 0.0, 1.0), _screenSize);

    float rot = angleBetweenPoints(p1, p2) + M_PI_2;

    if (rot > M_PI_2 || rot < -M_PI_2) { // un-readable labels
        rot += M_PI;
    }

    glm::vec2 screenPosition = (p1 + p2) / 2.0f;

    // don't display out of screen labels, and out of screen translations or not yet implemented in fstash
    alpha = screenPosition.x > _screenSize.x || screenPosition.x < 0 ? 0.0 : alpha;
    alpha = screenPosition.y > _screenSize.y || screenPosition.y < 0 ? 0.0 : alpha;

    m_buffer->transformID(m_id, screenPosition.x, screenPosition.y, rot, alpha);
}

