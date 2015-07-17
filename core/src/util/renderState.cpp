#include "renderState.h"

namespace RenderState {
    
    State<Blending> blending = State<Blending>();
    State<DepthWrite> depthWrite = State<DepthWrite>();
    State<ColorWrite> colorWrite = State<ColorWrite>();
    State<Culling> culling = State<Culling>();
    State<BlendingFunc> blendingFunc = State<BlendingFunc>();
    State<DepthTest> depthTest = State<DepthTest>();
    State<StencilTest> stencilTest = State<StencilTest>();
    State<StencilWrite> stencilWrite = State<StencilWrite>();
    State<StencilFunc> stencilFunc = State<StencilFunc>();
    State<StencilOp> stencilOp = State<StencilOp>();
    
    void configure() {
        blending.init(GL_FALSE);
        culling.init({GL_TRUE, GL_CCW, GL_BACK});
        depthTest.init(GL_TRUE);
        depthWrite.init(GL_TRUE);
        
        glDisable(GL_STENCIL_TEST);
        glDepthFunc(GL_LEQUAL);
        glClearDepthf(1.0);
        glDepthRangef(0.0, 1.0);
        glClearColor(0.3, 0.3, 0.3, 1.0);
    }
    
}
