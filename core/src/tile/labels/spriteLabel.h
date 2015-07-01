#pragma once

#include "label.h"

class SpriteLabel : public Label {
public:
    
    SpriteLabel(Label::Transform _transform, const glm::vec2& _size, const glm::vec2& _offset);
    
    void pushTransform() override;
    
protected:
    
    void updateBBoxes() override;
    
    glm::vec2 m_offset;
};