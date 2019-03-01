#include "annotation/annotation.h"

#include "gl/renderState.h"
#include "view/view.h"

namespace Tangram {

Annotation::~Annotation() {
    if (m_rendererInitialized) {
        m_renderer->deinitialize();
        m_rendererInitialized = false;
    }
}

void Annotation::draw(RenderState& rs, const View& _view) {

    if (!m_rendererInitialized) {
        m_renderer->initialize();
        m_rendererInitialized = true;
    }

    // Set up a context object for this frame.
    AnnotationViewState context;

    // Perform drawing with the custom renderer.
    m_renderer->render(context);

    // Invalidate the render state to handle any state changes from the renderer.
    rs.invalidate();
}

} // namespace Tangram
