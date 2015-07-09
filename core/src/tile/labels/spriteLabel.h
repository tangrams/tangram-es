#pragma once

#include "label.h"

class SpriteLabel : public Label {
public:
    
    struct AttributeOffsets {
        GLint m_size;
        GLvoid* m_memStart;
        GLvoid* m_position;
        GLvoid* m_rotation;
        GLvoid* m_alpha;
    };
    
    SpriteLabel(Label::Transform _transform, const glm::vec2& _size, const glm::vec2& _offset, AttributeOffsets _attribOffsets);
    
    void pushTransform(VboMesh& _mesh) override;
    
protected:
    
    void updateBBoxes() override;
    
    glm::vec2 m_offset;
    AttributeOffsets m_attribOffsets;
};