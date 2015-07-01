#pragma once

#include "label.h"

class SpriteLabel : public Label {
public:
    
    SpriteLabel(Label::Transform _transform);
    
    void pushTransform() override;
    
protected:
    
    void updateBBoxes() override;
};