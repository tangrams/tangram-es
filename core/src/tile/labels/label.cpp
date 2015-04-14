#include "label.h"
#include "labelContainer.h"

Label::Label(LabelTransform _transform, std::string _text, std::shared_ptr<TextBuffer> _buffer) :
    m_transform(_transform),
    m_text(_text),
    m_buffer(_buffer) {

    m_id = m_buffer->genTextID();
    m_visible = true;
    m_alpha = 1.0;
}

Label::~Label() {}

void Label::rasterize() {
    m_buffer->rasterize(m_text, m_id);
    
    m_bbox = m_buffer->getBBox(m_id);
    
    m_width = abs(m_bbox.z - m_bbox.x);
    m_height = abs(m_bbox.w - m_bbox.y);
}

void Label::updateBBoxes(glm::vec2 _screenPosition, float _rot) {
    glm::vec2 t = glm::vec2(cos(_rot), sin(_rot));
    glm::vec2 tperp = glm::vec2(-t.y, t.x);
    
    _screenPosition = _screenPosition + t * (m_width / 2);
    _screenPosition = _screenPosition - tperp * (m_height / 8);
    
    m_obb = isect2d::OBB(_screenPosition.x, _screenPosition.y, _rot, m_width, m_height);
    
    m_aabb = m_obb.getExtent();
}

void Label::updateScreenPosition(const glm::mat4& _mvp, const glm::vec2& _screenSize, float _dt) {
    
    if (!m_visible) {
        m_buffer->transformID(m_id, 0, 0, 0, 0.0);
        return;
    }
    
    glm::vec2 screenPosition;
    float rot = 0;
    
    if (m_transform.m_modelPosition1 != m_transform.m_modelPosition2) { // label fitting to a line
        glm::vec2 p1 = worldToScreenSpace(_mvp, glm::vec4(m_transform.m_modelPosition1, 0.0, 1.0), _screenSize);
        glm::vec2 p2 = worldToScreenSpace(_mvp, glm::vec4(m_transform.m_modelPosition2, 0.0, 1.0), _screenSize);
        
        glm::vec2 p1p2 = p2 - p1;
        glm::vec2 t = glm::normalize(-p1p2);
        
        float length = glm::length(p1p2);
        
        float exceedHeuristic = 40; // default heuristic : 40%
        
        if (m_width > length) {
            float exceed = (1 - (length / m_width)) * 100;
            m_visible = exceed < exceedHeuristic;
        }
        
        rot = angleBetweenPoints(p1, p2) + M_PI_2;
        
        if (rot > M_PI_2 || rot < -M_PI_2) { // un-readable labels
            rot += M_PI;
        }
        
        screenPosition = (p1 + p2) / 2.0f;
        screenPosition += t * (m_width / 2);
    } else { // label sticking to a point
        screenPosition = worldToScreenSpace(_mvp, glm::vec4(m_transform.m_modelPosition1, 0.0, 1.0), _screenSize);
        screenPosition.x -= m_width / 2;
    }
    
    // don't display out of screen labels, and out of screen translations or not yet implemented in fstash
    bool outOfScreen;
    
    outOfScreen = screenPosition.x > _screenSize.x || screenPosition.x < 0;
    outOfScreen = outOfScreen || screenPosition.y > _screenSize.y || screenPosition.y < 0;
    
    m_alpha = !outOfScreen && m_visible ? 1.0 : 0.0;
    
    m_buffer->transformID(m_id, screenPosition.x, screenPosition.y, rot, m_alpha);
    
    updateBBoxes(screenPosition, rot);
}

