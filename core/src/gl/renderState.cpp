#include "renderState.h"

namespace Tangram {

namespace RenderState {
    
    Blending blending;
    DepthTest depthTest;
    StencilTest stencilTest;
    Culling culling;
    DepthWrite depthWrite;
    BlendingFunc blendingFunc;
    StencilWrite stencilWrite;
    StencilFunc stencilFunc;
    StencilOp stencilOp;
    ColorWrite colorWrite;
    FrontFace frontFace;
    CullFace cullFace;

    void configure() {
        blending.init(GL_FALSE);
        culling.init(GL_TRUE);
        cullFace.init(GL_BACK);
        frontFace(GL_CCW);
        depthTest.init(GL_TRUE);
        depthWrite.init(GL_TRUE);

        glDisable(GL_STENCIL_TEST);
        glDepthFunc(GL_LEQUAL);
        glClearDepthf(1.0);
        glDepthRangef(0.0, 1.0);
        glClearColor(0.3, 0.3, 0.3, 1.0);
    }

}

}
