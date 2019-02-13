//
//  IOSCustomRenderer.mm
//  tangram
//
//  Created by Varun Talwar on 7/13/18.
//

#import "TGCustomRenderer.h"
#import "IOSCustomRenderer.h"

namespace Tangram {

IOSCustomRenderer::IOSCustomRenderer(id<TGCustomRenderer> customRenderer) : m_tgCustomRenderer(customRenderer) {}

void IOSCustomRenderer::initialize() {
    [m_tgCustomRenderer prepare];
}

void IOSCustomRenderer::render(const Tangram::CustomRenderContext& context) {
    TGCustomRendererContext ctx;
    ctx.width = context.width;
    ctx.height = context.height;
    ctx.latitude = context.latitude;
    ctx.longitude = context.longitude;
    ctx.fieldOfView = context.fieldOfView;
    ctx.tilt = context.tilt;
    ctx.rotation = context.rotation;
    ctx.zoom = context.zoom;
    [m_tgCustomRenderer drawWithContext:ctx];
}

void IOSCustomRenderer::deinitialize() {
    [m_tgCustomRenderer complete];
}

}
