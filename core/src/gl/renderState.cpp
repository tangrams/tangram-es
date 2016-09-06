#include "renderState.h"

#include "platform.h"
#include "vertexLayout.h"
#include "gl/error.h"
#include "gl/hardware.h"

#include <limits>

namespace Tangram {

RenderState::RenderState() {

    m_blending = { 0, false };
    m_culling = { 0, false };
    m_depthMask = { 0, false };
    m_depthTest = { 0, false };
    m_stencilTest = { 0, false };
    m_blendingFunc = { 0, 0, false };
    m_stencilMask = { 0, false };
    m_stencilFunc = { 0, 0, 0, false };
    m_stencilOp = { 0, 0, 0, false };
    m_colorMask = { 0, 0, 0, 0, false };
    m_frontFace = { 0, false };
    m_cullFace = { 0, false };
    m_vertexBuffer = { 0, false };
    m_indexBuffer = { 0, false };
    m_program = { 0, false };
    m_clearColor = { 0., 0., 0., 0., false };
    m_texture = { 0, 0, false };
    m_textureUnit = { 0, false };
    m_framebuffer = { 0, false };
    m_viewport = { 0, 0, 0, 0, false };

}

GLuint RenderState::getTextureUnit(GLuint _unit) {
    return GL_TEXTURE0 + _unit;
}

RenderState::~RenderState() {

    deleteQuadIndexBuffer();

}

void RenderState::invalidate() {

    m_blending.set = false;
    m_blendingFunc.set = false;
    m_clearColor.set = false;
    m_colorMask.set = false;
    m_cullFace.set = false;
    m_culling.set = false;
    m_depthTest.set = false;
    m_depthMask.set = false;
    m_frontFace.set = false;
    m_stencilTest.set = false;
    m_stencilMask.set = false;
    m_program.set = false;
    m_indexBuffer.set = false;
    m_vertexBuffer.set = false;
    m_texture.set = false;
    m_textureUnit.set = false;
    m_viewport.set = false;
    m_framebuffer.set = false;

    attributeBindings.fill(0);

    GL_CHECK(glDepthFunc(GL_LESS));
    GL_CHECK(glClearDepthf(1.0));
    GL_CHECK(glDepthRangef(0.0, 1.0));

}

void RenderState::cacheDefaultFramebuffer() {
    GLint handle;

    GL_CHECK(glGetIntegerv(GL_FRAMEBUFFER_BINDING, &handle));
    m_framebuffer = { (GLuint)handle, false };
}

void RenderState::increaseGeneration() {
    generateQuadIndexBuffer();
    m_validGeneration++;
}

bool RenderState::isValidGeneration(int _generation) {
    return _generation == m_validGeneration;
}

int RenderState::generation() {
    return m_validGeneration;
}

int RenderState::nextAvailableTextureUnit() {
    if (m_nextTextureUnit >= Hardware::maxCombinedTextureUnits) {
        LOGE("Too many combined texture units are being used");
        LOGE("GPU supports %d combined texture units", Hardware::maxCombinedTextureUnits);
    }

    return ++m_nextTextureUnit;
}

void RenderState::releaseTextureUnit() {
    m_nextTextureUnit--;
}

int RenderState::currentTextureUnit() {
    return m_nextTextureUnit;
}

void RenderState::resetTextureUnit() {
    m_nextTextureUnit = 0;
}

inline void setGlFlag(GLenum flag, GLboolean enable) {
    if (enable) {
        GL_CHECK(glEnable(flag));
    } else {
        GL_CHECK(glDisable(flag));
    }
}

bool RenderState::blending(GLboolean enable) {
    if (!m_blending.set || m_blending.enabled != enable) {
        m_blending = { enable, true };
        setGlFlag(GL_BLEND, enable);
        return false;
    }
    return true;
}

bool RenderState::blendingFunc(GLenum sfactor, GLenum dfactor) {
    if (!m_blendingFunc.set || m_blendingFunc.sfactor != sfactor || m_blendingFunc.dfactor != dfactor) {
        m_blendingFunc = { sfactor, dfactor, true };
        GL_CHECK(glBlendFunc(sfactor, dfactor));
        return false;
    }
    return true;
}

bool RenderState::clearColor(GLclampf r, GLclampf g, GLclampf b, GLclampf a) {
    if (!m_clearColor.set || m_clearColor.r != r || m_clearColor.g != g || m_clearColor.b != b || m_clearColor.a != a) {
        m_clearColor = { r, g, b, a, true };
        GL_CHECK(glClearColor(r, g, b, a));
        return false;
    }
    return true;
}

bool RenderState::colorMask(GLboolean r, GLboolean g, GLboolean b, GLboolean a) {
    if (!m_colorMask.set || m_colorMask.r != r || m_colorMask.g != g || m_colorMask.b != b || m_colorMask.a != a) {
        m_colorMask = { r, g, b, a, true };
        GL_CHECK(glColorMask(r, g, b, a));
        return false;
    }
    return true;
}

bool RenderState::cullFace(GLenum face) {
    if (!m_cullFace.set || m_cullFace.face != face) {
        m_cullFace = { face, true };
        GL_CHECK(glCullFace(face));
        return false;
    }
    return true;
}

bool RenderState::culling(GLboolean enable) {
    if (!m_culling.set || m_culling.enabled != enable) {
        m_culling = { enable, true };
        setGlFlag(GL_CULL_FACE, enable);
        return false;
    }
    return true;
}

bool RenderState::depthTest(GLboolean enable) {
    if (!m_depthTest.set || m_depthTest.enabled != enable) {
        m_depthTest = { enable, true };
        setGlFlag(GL_DEPTH_TEST, enable);
        return false;
    }
    return true;
}

bool RenderState::depthMask(GLboolean enable) {
    if (!m_depthMask.set || m_depthMask.enabled != enable) {
        m_depthMask = { enable, true };
        GL_CHECK(glDepthMask(enable));
        return false;
    }
    return true;
}

bool RenderState::frontFace(GLenum face) {
    if (!m_frontFace.set || m_frontFace.face != face) {
        m_frontFace = { face, true };
        GL_CHECK(glFrontFace(face));
        return false;
    }
    return true;
}

bool RenderState::stencilMask(GLuint mask) {
    if (!m_stencilMask.set || m_stencilMask.mask != mask) {
        m_stencilMask = { mask, true };
        GL_CHECK(glStencilMask(mask));
        return false;
    }
    return true;
}

bool RenderState::stencilFunc(GLenum func, GLint ref, GLuint mask) {
    if (!m_stencilFunc.set || m_stencilFunc.func != func || m_stencilFunc.ref != ref || m_stencilFunc.mask != mask) {
        m_stencilFunc = { func, ref, mask, true };
        GL_CHECK(glStencilFunc(func, ref, mask));
        return false;
    }
    return true;
}

bool RenderState::stencilOp(GLenum sfail, GLenum spassdfail, GLenum spassdpass) {
    if (!m_stencilOp.set || m_stencilOp.sfail != sfail || m_stencilOp.spassdfail != spassdfail || m_stencilOp.spassdpass != spassdpass) {
        m_stencilOp = { sfail, spassdfail, spassdpass, true };
        GL_CHECK(glStencilOp(sfail, spassdfail, spassdpass));
        return false;
    }
    return true;
}

bool RenderState::stencilTest(GLboolean enable) {
    if (!m_stencilTest.set || m_stencilTest.enabled != enable) {
        m_stencilTest = { enable, true };
        setGlFlag(GL_STENCIL_TEST, enable);
        return false;
    }
    return true;
}

bool RenderState::shaderProgram(GLuint program) {
    if (!m_program.set || m_program.program != program) {
        m_program = { program, true };
        GL_CHECK(glUseProgram(program));
        return false;
    }
    return true;
}

bool RenderState::texture(GLenum target, GLuint handle) {
    if (!m_texture.set || m_texture.target != target || m_texture.handle != handle) {
        m_texture = { target, handle, true };
        GL_CHECK(glBindTexture(target, handle));
        return false;
    }
    return true;
}

bool RenderState::textureUnit(GLuint unit) {
    if (!m_textureUnit.set || m_textureUnit.unit != unit) {
        m_textureUnit = { unit, true };
        // Our cached texture handle is irrelevant on the new unit, so unset it.
        m_texture.set = false;
        GL_CHECK(glActiveTexture(getTextureUnit(unit)));
        return false;
    }
    return true;
}

bool RenderState::vertexBuffer(GLuint handle) {
    if (!m_vertexBuffer.set || m_vertexBuffer.handle != handle) {
        m_vertexBuffer = { handle, true };
        GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, handle));
        return false;
    }
    return true;
}

bool RenderState::indexBuffer(GLuint handle) {
    if (!m_indexBuffer.set || m_indexBuffer.handle != handle) {
        m_indexBuffer = { handle, true };
        GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, handle));
        return false;
    }
    return true;
}

void RenderState::vertexBufferUnset(GLuint handle) {
    if (m_vertexBuffer.handle == handle) {
        m_vertexBuffer.set = false;
    }
}

void RenderState::indexBufferUnset(GLuint handle) {
    if (m_indexBuffer.handle == handle) {
        m_indexBuffer.set = false;
    }
}

void RenderState::shaderProgramUnset(GLuint program) {
    if (m_program.program == program) {
        m_program.set = false;
    }
}

void RenderState::textureUnset(GLenum target, GLuint handle) {
    if (m_texture.handle == handle) {
        m_texture.set = false;
    }
}

void RenderState::framebufferUnset(GLuint handle) {
    if (m_framebuffer.handle == handle) {
        m_framebuffer.set = false;
    }
}

GLuint RenderState::getQuadIndexBuffer() {
    if (m_quadIndexBuffer == 0) {
        generateQuadIndexBuffer();
    }
    return m_quadIndexBuffer;
}

void RenderState::deleteQuadIndexBuffer() {
    indexBufferUnset(m_quadIndexBuffer);
    GL_CHECK(glDeleteBuffers(1, &m_quadIndexBuffer));
    m_quadIndexBuffer = 0;
}

void RenderState::generateQuadIndexBuffer() {

    std::vector<GLushort> indices;
    indices.reserve(MAX_QUAD_VERTICES / 4 * 6);

    for (size_t i = 0; i < MAX_QUAD_VERTICES; i += 4) {
        indices.push_back(i + 2);
        indices.push_back(i + 0);
        indices.push_back(i + 1);
        indices.push_back(i + 1);
        indices.push_back(i + 3);
        indices.push_back(i + 2);
    }

    GL_CHECK(glGenBuffers(1, &m_quadIndexBuffer));
    indexBuffer(m_quadIndexBuffer);
    GL_CHECK(glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLushort),
                 reinterpret_cast<GLbyte*>(indices.data()), GL_STATIC_DRAW));

}

bool RenderState::framebuffer(GLuint handle) {
    if (!m_framebuffer.set || m_framebuffer.handle != handle) {
        m_framebuffer = { handle, true };
        GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, handle));
        return false;
    }
    return true;
}

bool RenderState::viewport(GLint x, GLint y, GLsizei width, GLsizei height) {
    if (!m_viewport.set || m_viewport.x != x || m_viewport.y != y
      || m_viewport.width != width || m_viewport.height != height) {
        m_viewport = { x, y, width, height, true };
        GL_CHECK(glViewport(x, y, width, height));
        return false;
    }
    return true;
}

void RenderState::saveFramebufferState() {
    m_savedFrameBufferState = m_framebuffer;
    m_savedViewportState = m_viewport;
}

void RenderState::applySavedFramebufferState() {
    framebuffer(m_savedFrameBufferState.handle);
    viewport(m_savedViewportState.x, m_savedViewportState.y,
             m_savedViewportState.width, m_savedViewportState.height);
}

} // namespace Tangram
