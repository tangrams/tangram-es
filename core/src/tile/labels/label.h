#pragma once

#include "glm/glm.hpp"
#include "text/fontContext.h"
#include "text/textBuffer.h"
#include "isect2d.h"
#include <string>

struct LabelTransform {
    glm::vec2 m_modelPosition1;
    glm::vec2 m_modelPosition2;
    float m_alpha;
};

class Label {

public:

    Label(LabelTransform _transform, std::string _text, std::shared_ptr<TextBuffer> _buffer);
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
    
    
    
    bool isOutOfScreen() { return m_outOfScreen; }

private:

    LabelTransform m_transform;
    std::string m_text;
    std::shared_ptr<TextBuffer> m_buffer; // the buffer in which this label text id is associated to
    fsuint m_id;
    
    isect2d::OBB m_obb;
    isect2d::AABB m_aabb;
    
    bool m_outOfScreen;
    
    glm::vec4 m_bbox;
    float m_width;
    float m_height;

};
