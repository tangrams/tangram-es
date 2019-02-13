#include "style/customStyle.h"

#include "gl/renderState.h"
#include "view/view.h"

namespace Tangram {

CustomStyle::~CustomStyle() {
    if (m_rendererInitialized) {
        m_renderer->deinitialize();
        m_rendererInitialized = false;
    }
}

void CustomStyle::draw(RenderState& rs, const View& _view) {

    if (!m_rendererInitialized) {
        m_renderer->initialize();
        m_rendererInitialized = true;
    }

    // Set up a context object for this frame.
    CustomRenderContext context;
    context.width = _view.getWidth();
    context.height = _view.getHeight();
    context.zoom = _view.getZoom();
    context.rotation = _view.getRoll();
    context.tilt = _view.getPitch();
    context.fieldOfView = _view.getFieldOfView();

    auto meters = _view.getPosition();
    auto lnglat = MapProjection::projectedMetersToLngLat({ meters.x, meters.y });
    context.longitude = lnglat.longitude;
    context.latitude = lnglat.latitude;

    // Perform drawing with the custom renderer.
    m_renderer->render(context);

    // Invalidate the render state to handle any state changes from the renderer.
    rs.invalidate();
}

} // namespace Tangram
