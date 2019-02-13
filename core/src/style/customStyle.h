#pragma once

#include "customRenderer.h"
#include <stdint.h>

namespace Tangram {

class RenderState;
class View;

class CustomStyle {

public:
    CustomStyle(CustomRenderer* _renderer, uint32_t _beforeId)
    : m_renderer(_renderer), m_beforeId(_beforeId) {}
    
    ~CustomStyle();

    void draw(RenderState& rs, const View& _view);

    const CustomRenderer* renderer() const { return m_renderer; }
    uint32_t beforeId() { return m_beforeId; }
    
private:
    CustomRenderer* m_renderer;
    uint32_t m_beforeId;
    bool m_rendererInitialized = false;
};

} // namespace Tangram
