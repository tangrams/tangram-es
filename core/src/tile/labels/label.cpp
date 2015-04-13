#include "label.h"
#include "labelContainer.h"

Label::Label(LabelTransform _transform, std::string _text, std::shared_ptr<TextBuffer> _buffer) :
    m_transform(_transform),
    m_text(_text),
    m_buffer(_buffer) {

    m_id = m_buffer->genTextID();
    m_outOfScreen = false;
}

Label::~Label() {}

void Label::rasterize() {
    m_buffer->rasterize(m_text, m_id);
    
    m_bbox = m_buffer->getBBox(m_id);
    
    m_width = abs(m_bbox.z - m_bbox.x);
    m_height = abs(m_bbox.w - m_bbox.y);
}

void Label::updateTransform(const LabelTransform& _transform, const glm::mat4& _mvp, const glm::vec2& _screenSize) {
    m_transform = _transform;
    m_outOfScreen = false;

    float alpha = m_transform.m_alpha;
    glm::vec2 screenPosition;
    float rot = 0;
    
    if (m_transform.m_modelPosition1 != m_transform.m_modelPosition2) {
        glm::vec2 p1 = worldToScreenSpace(_mvp, glm::vec4(m_transform.m_modelPosition1, 0.0, 1.0), _screenSize);
        glm::vec2 p2 = worldToScreenSpace(_mvp, glm::vec4(m_transform.m_modelPosition2, 0.0, 1.0), _screenSize);

        rot = angleBetweenPoints(p1, p2) + M_PI_2;

        if (rot > M_PI_2 || rot < -M_PI_2) { // un-readable labels
            rot += M_PI;
        }
        
        screenPosition = (p1 + p2) / 2.0f;
    } else {
        screenPosition = worldToScreenSpace(_mvp, glm::vec4(m_transform.m_modelPosition1, 0.0, 1.0), _screenSize);
    }

    // don't display out of screen labels, and out of screen translations or not yet implemented in fstash
    
    m_outOfScreen = screenPosition.x > _screenSize.x || screenPosition.x < 0;
    m_outOfScreen = m_outOfScreen || screenPosition.y > _screenSize.y || screenPosition.y < 0;
    
    alpha = m_outOfScreen ? 0.0 : alpha;

    m_buffer->transformID(m_id, screenPosition.x, screenPosition.y, rot, alpha);
    
    if (m_outOfScreen)
        return;
    
    glm::vec2 t = glm::vec2(cos(rot), sin(rot));
    glm::vec2 tperp = glm::vec2(-t.y, t.x);
    
    screenPosition = screenPosition + t * (m_width / 2);
    screenPosition = screenPosition - tperp * (m_height / 8);
    
    m_obb = isect2d::OBB(screenPosition.x, screenPosition.y, rot, m_width, m_height);
    
    m_aabb = m_obb.getExtent();
}

