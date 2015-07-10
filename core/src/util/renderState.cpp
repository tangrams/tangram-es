#include "renderState.h"

namespace RenderState {
    
    void configure() {
        blending.init(GL_FALSE);
        blendingFunc.init({GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA});
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
