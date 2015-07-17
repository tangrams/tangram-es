#pragma once

#include "label.h"

class SpriteLabel : public Label {
public:
    
    SpriteLabel(Label::Transform _transform, const glm::vec2& _size, const glm::vec2& _offset, size_t _bufferOffset);
    
    void pushTransform(VboMesh& _mesh) override;
    
protected:
    
    void updateBBoxes() override;
    
    glm::vec2 m_offset;

    size_t m_bufferOffset;
};
