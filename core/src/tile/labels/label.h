#pragma once

#include "glm/glm.hpp"
#include "text/fontContext.h"
#include "text/textBuffer.h"
#include "isect2d.h"
#include <string>

struct LabelTransform {
    glm::vec2 m_modelPosition1;
    glm::vec2 m_modelPosition2;
    
    glm::vec2 m_screenPosition;
    
    float m_alpha;
    float m_rotation;
};
class Label {

public:
    
    enum class Type {
        POINT,
        LINE,
        DEBUG
    };


    Label(LabelTransform _transform, std::string _text, std::shared_ptr<TextBuffer> _buffer, Type _type);
    ~Label();

    /* Call the font context to rasterize the label string */
    void rasterize();

    LabelTransform getTransform() const { return m_transform; }

    /* Update the transform of the label in world space, and project it to screen space */
    void updateTransform(const LabelTransform& _transform, const glm::mat4& _mvp, const glm::vec2& _screenSize);
    
    /* gets the oriented bounding box of the label */
    const isect2d::OBB& getOBB() const { return m_obb; }
    
    /* gets the extent of the oriented bounding box of the label */
    const isect2d::AABB& getAABB() const { return m_aabb; }
    
    std::string getText() { return m_text; }
    
    void setVisible(bool _visible);
    
    bool isOutOfScreen() const { return m_outOfScreen; }
    
    bool isVisible() const { return m_visible; }
    
    void update(const glm::mat4& _mvp, const glm::vec2& _screenSize, float _dt);

    void pushTransform();
    
    void updateScreenTransform(const glm::mat4& _mvp, const glm::vec2& _screenSize);
    
    Type getType() const { return m_type; }

private:
    
    void updateBBoxes();

    Type m_type;
    LabelTransform m_transform;
    std::string m_text;
    std::shared_ptr<TextBuffer> m_buffer; // the buffer in which this label text id is associated to
    fsuint m_id;
    
    bool m_dirty;
    bool m_visible;
    bool m_outOfScreen;
    
    isect2d::OBB m_obb;
    isect2d::AABB m_aabb;
    glm::vec4 m_bbox;
    
    float m_width;
    float m_height;

};
