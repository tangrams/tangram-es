#pragma once

#include "annotation/annotationRenderer.h"
#include <stdint.h>

namespace Tangram {

class RenderState;
class View;

class Annotation {

public:
    explicit Annotation(AnnotationRenderer* _renderer)
    : m_renderer(_renderer) {}
    
    ~Annotation();

    void draw(RenderState& rs, const View& _view);

    const AnnotationRenderer* renderer() const { return m_renderer; }
    
private:
    AnnotationRenderer* m_renderer;
    bool m_rendererInitialized = false;
};

} // namespace Tangram
