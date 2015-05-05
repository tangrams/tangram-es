#include "label.h"

Label::Label(LabelTransform _transform, std::string _text, std::shared_ptr<TextBuffer> _buffer, Type _type) :
    m_type(_type),
    m_transform(_transform),
    m_text(_text),
    m_buffer(_buffer) {

    m_id = m_buffer->genTextID();
    m_visible = true;
    m_dirty = false;
}

Label::~Label() {}

void Label::rasterize() {
    m_buffer->rasterize(m_text, m_id);
    
    m_bbox = m_buffer->getBBox(m_id);
    
    m_width = std::abs(m_bbox.z - m_bbox.x);
    m_height = std::abs(m_bbox.w - m_bbox.y);
}

void Label::setVisible(bool _visible) {
    m_visible = _visible;
    m_dirty = true;
}

void Label::updateBBoxes() {
    glm::vec2 t = glm::vec2(cos(m_transform.m_rotation), sin(m_transform.m_rotation));
    glm::vec2 tperp = glm::vec2(-t.y, t.x);
    glm::vec2 obbCenter;
    
    obbCenter = m_transform.m_screenPosition + t * (m_width / 2) - tperp * (m_height / 8);
    
    m_obb = isect2d::OBB(obbCenter.x, obbCenter.y, m_transform.m_rotation, m_width, m_height);
    m_aabb = m_obb.getExtent();
}

void Label::pushTransform() {
    
    if (m_dirty) {
        if (!m_visible || m_outOfScreen) {
            m_buffer->transformID(m_id, 0, 0, 0, 0);
        } else {
            m_buffer->transformID(m_id, m_transform.m_screenPosition.x, m_transform.m_screenPosition.y, m_transform.m_rotation, 1.0);
        }
        m_dirty = false;
    }

}

void Label::updateScreenTransform(const glm::mat4& _mvp, const glm::vec2& _screenSize) {
    
    glm::vec2 screenPosition;
    float rot = 0;
    
    switch (m_type) {
        case Type::DEBUG:
        case Type::POINT:
        {
            glm::vec4 v1 = worldToClipSpace(_mvp, glm::vec4(m_transform.m_modelPosition1, 0.0, 1.0));
            
            if (v1.w <= 0) {
                m_visible = false;
            }
            
            screenPosition = clipToScreenSpace(v1, _screenSize);
            
            // center on half the width
            screenPosition.x -= m_width / 2;
            
            break;
        }
        case Type::LINE:
        {
            // project label position from mercator world space to clip coordinates
            glm::vec4 v1 = worldToClipSpace(_mvp, glm::vec4(m_transform.m_modelPosition1, 0.0, 1.0));
            glm::vec4 v2 = worldToClipSpace(_mvp, glm::vec4(m_transform.m_modelPosition2, 0.0, 1.0));
            
            // check whether the label is behind the camera using the perspective division factor
            if (v1.w <= 0 || v2.w <= 0) {
                m_visible = false;
            }
            
            // project to screen space
            glm::vec2 p1 = clipToScreenSpace(v1, _screenSize);
            glm::vec2 p2 = clipToScreenSpace(v2, _screenSize);
            
            rot = angleBetweenPoints(p1, p2) + M_PI_2;
            
            if (rot > M_PI_2 || rot < -M_PI_2) { // un-readable labels
                rot += M_PI;
            } else {
                std::swap(p1, p2);
            }
            
            glm::vec2 p1p2 = p2 - p1;
            glm::vec2 t = glm::normalize(-p1p2);
            
            float length = glm::length(p1p2);
            
            float exceedHeuristic = 40; // default heuristic : 40%
            
            if (m_width > length) {
                float exceed = (1 - (length / m_width)) * 100;
                m_visible &= exceed < exceedHeuristic;
            }
            
            screenPosition = (p1 + p2) / 2.0f;
            
            // translate by half the width
            screenPosition += t * (m_width / 2);

            break;
        }
    
        default:
            break;
    }
    
    m_transform.m_screenPosition = screenPosition;
    m_transform.m_rotation = rot;
    
    m_outOfScreen = screenPosition.x > _screenSize.x || screenPosition.x < 0;
    m_outOfScreen = m_outOfScreen || screenPosition.y > _screenSize.y || screenPosition.y < 0;
    
    m_dirty = true;
}

void Label::update(const glm::mat4& _mvp, const glm::vec2& _screenSize, float _dt) {

    updateScreenTransform(_mvp, _screenSize);
    updateBBoxes();
    
}

