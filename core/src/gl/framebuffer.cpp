#include "framebuffer.h"

#include "gl/texture.h"
#include "gl/renderState.h"
#include "gl/error.h"
#include "log.h"
#include "glm/vec2.hpp"

namespace Tangram {

FrameBuffer::FrameBuffer(int _width, int _height, bool _colorRenderBuffer) :
    m_glFrameBufferHandle(0),
    m_generation(-1),
    m_valid(false),
    m_colorRenderBuffer(_colorRenderBuffer),
    m_width(_width), m_height(_height) {

}

bool FrameBuffer::applyAsRenderTarget(RenderState& _rs, glm::vec4 _clearColor) {

    if (!m_glFrameBufferHandle) {
        init(_rs);
    }

    if (!m_valid) {
        return false;
    }

    FrameBuffer::apply(_rs, m_glFrameBufferHandle, {m_width, m_height}, _clearColor);

    return true;
}

void FrameBuffer::apply(RenderState& _rs, GLuint _handle, glm::vec2 _viewport, glm::vec4 _clearColor) {

    _rs.framebuffer(_handle);
    _rs.viewport(0, 0, _viewport.x, _viewport.y);

    _rs.clearColor(_clearColor.r / 255.f,
                   _clearColor.g / 255.f,
                   _clearColor.b / 255.f,
                   _clearColor.a / 255.f);

    // Enable depth testing
    _rs.depthMask(GL_TRUE);

    // Setup raster state
    _rs.culling(GL_TRUE);
    _rs.cullFace(GL_BACK);

    GL::clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void FrameBuffer::bind(RenderState& _rs) const {

    if (m_valid) {
        _rs.framebuffer(m_glFrameBufferHandle);
    }
}

GLuint FrameBuffer::readAt(float _normalizedX, float _normalizedY) const {

    glm::vec2 position(_normalizedX * m_width, _normalizedY * m_height);

    GLuint pixel;
    GL::readPixels(floorf(position.x), floorf(position.y),
                   1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &pixel);

    return pixel;
}

void FrameBuffer::init(RenderState& _rs) {

    GL::genFramebuffers(1, &m_glFrameBufferHandle);

    _rs.framebuffer(m_glFrameBufferHandle);

    // Setup color render target
    if (m_colorRenderBuffer) {
        GL::genRenderbuffers(1, &m_glColorRenderBufferHandle);
        GL::bindRenderbuffer(GL_RENDERBUFFER, m_glColorRenderBufferHandle);
        GL::renderbufferStorage(GL_RENDERBUFFER, GL_RGBA8_OES,
                                m_width, m_height);

        GL::framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                    GL_RENDERBUFFER, m_glColorRenderBufferHandle);
    } else {
        TextureOptions options =
            {GL_RGBA, GL_RGBA,
            {GL_NEAREST, GL_NEAREST},
            {GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE}
        };

        m_texture = std::make_unique<Texture>(m_width, m_height, options);
        m_texture->update(_rs, 0);

        GL::framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                 GL_TEXTURE_2D, m_texture->getGlHandle(), 0);
    }

    {
        // Create depth render buffer
        GL::genRenderbuffers(1, &m_glDepthRenderBufferHandle);
        GL::bindRenderbuffer(GL_RENDERBUFFER, m_glDepthRenderBufferHandle);
        GL::renderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16,
                                m_width, m_height);

        GL::framebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                    GL_RENDERBUFFER, m_glDepthRenderBufferHandle);
    }

    GLenum status = GL::checkFramebufferStatus(GL_FRAMEBUFFER);
    GL_CHECK();

    if (status != GL_FRAMEBUFFER_COMPLETE) {
        LOGE("Framebuffer status is incomplete:");

        switch (status) {
            case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
                LOGE("\tGL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT");
                break;
            case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
                LOGE("\tGL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT");
                break;
            case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
                LOGE("\tGL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS");
                break;
            case GL_FRAMEBUFFER_UNSUPPORTED:
                LOGE("\tGL_FRAMEBUFFER_UNSUPPORTED");
                break;
            default:
                LOGE("\tUnknown framebuffer issue");
                break;
        }
    } else {
        m_valid = true;
    }

    m_generation = _rs.generation();

    m_disposer = Disposer(_rs);
}

FrameBuffer::~FrameBuffer() {

    int generation = m_generation;
    GLuint glHandle = m_glFrameBufferHandle;

    m_disposer([=](RenderState& rs) {
        if (rs.isValidGeneration(generation)) {
            rs.framebufferUnset(glHandle);

            GL::deleteFramebuffers(1, &glHandle);
        }
    });
}

}
