#pragma once

#include "label.h"

class SpriteLabel : public Label {
public:
    
    SpriteLabel(Label::Transform _transform, glm::vec2& _size);
    
    void pushTransform() override;
    
protected:
    
    void updateBBoxes() override;
};