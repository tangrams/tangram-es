#include "spriteLabel.h"

SpriteLabel::SpriteLabel(Label::Transform _transform) : Label(_transform, Label::Type::POINT) {
    // TODO : get size
    m_dim = {32, 32};
}

void SpriteLabel::pushTransform() {
    // TODO : update vbo mesh
}