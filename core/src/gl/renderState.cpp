#include "renderState.h"

#include "platform.h"
#include "vertexLayout.h"
#include "gl/hardware.h"

namespace Tangram {

 // Incremented when the GL context is invalidated
static int s_validGeneration;
static int s_textureUnit;

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

    VertexBuffer vertexBuffer;
    IndexBuffer indexBuffer;

    ShaderProgram shaderProgram;

    TextureUnit textureUnit;
    Texture texture;

    ClearColor clearColor;

    GLuint getTextureUnit(GLuint _unit) {
        return GL_TEXTURE0 + _unit;
    }

    static size_t max = std::numeric_limits<size_t>::max();

    void bindVertexBuffer(GLuint _id) {
        GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, _id));
    }

    void bindIndexBuffer(GLuint _id) {
        GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _id));
    }

    void activeTextureUnit(GLuint _unit) {
        // current texture unit is changing, invalidate current texture binding:
        texture.init(GL_TEXTURE_2D, max, false);
        GL_CHECK(glActiveTexture(getTextureUnit(_unit)));
    }

    void bindTexture(GLenum _target, GLuint _textureId) {
        GL_CHECK(glBindTexture(_target, _textureId));
    }

    void invalidate() {
        s_textureUnit = -1;
        VertexLayout::clearCache();

        blending.init(GL_FALSE);
        blendingFunc.init(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        culling.init(GL_TRUE);
        cullFace.init(GL_BACK);
        frontFace.init(GL_CCW);
        depthTest.init(GL_TRUE);
        depthWrite.init(GL_TRUE);

        GL_CHECK(glDisable(GL_STENCIL_TEST));
        GL_CHECK(glDepthFunc(GL_LESS));
        GL_CHECK(glClearDepthf(1.0));
        GL_CHECK(glDepthRangef(0.0, 1.0));


        clearColor.init(0.0, 0.0, 0.0, 0.0);
        shaderProgram.init(max, false);
        vertexBuffer.init(max, false);
        indexBuffer.init(max, false);
        texture.init(GL_TEXTURE_2D, max, false);
        texture.init(GL_TEXTURE_CUBE_MAP, max, false);
        textureUnit.init(max, false);
    }

    void increaseGeneration() {
        s_validGeneration++;
    }

    bool isValidGeneration(int _generation) {
        return _generation == s_validGeneration;
    }

    int generation() {
        return s_validGeneration;
    }

    int nextAvailableTextureUnit() {
        if (s_textureUnit + 1 > Hardware::maxCombinedTextureUnits) {
            LOGE("Too many combined texture units are being used");
            LOGE("GPU supports %d combined texture units", Hardware::maxCombinedTextureUnits);
        }

        return ++s_textureUnit;
    }

    void releaseTextureUnit() {
        s_textureUnit--;
    }

    int currentTextureUnit() {
        return s_textureUnit;
    }

    void resetTextureUnit() {
        s_textureUnit = -1;
    }
}

}
