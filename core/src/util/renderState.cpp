#include "renderState.h"

namespace RenderState {
    
    State<Blending> blending = State<Blending>();
    State<ColorWrite> colorWrite = State<ColorWrite>();
    State<Culling> culling = State<Culling>();
    State<DepthTest> depthTest = State<DepthTest>();
    State<StencilTest> stencilTest = State<StencilTest>();

    StateWrap<GLboolean> depthWrite(glDepthMask);
    StateWrap<GLenum, GLenum> blendingFunc(glBlendFunc);
    StateWrap<GLuint> stencilWrite(glStencilMask);
    StateWrap<GLenum, GLint, GLuint> stencilFunc(glStencilFunc);
    StateWrap<GLenum, GLenum, GLenum> stencilOp(glStencilOp);
    
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
