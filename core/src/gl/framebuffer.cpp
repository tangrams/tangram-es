#include "framebuffer.h"

#include "gl/texture.h"
#include "gl/renderState.h"
#include "gl/error.h"

namespace Tangram {

FrameBuffer::FrameBuffer(bool _colorRenderBuffer) :
    m_glFrameBufferHandle(0),
    m_generation(-1),
    m_valid(false),
    m_colorRenderBuffer(_colorRenderBuffer),
    m_width(0), m_height(0) {

}

bool FrameBuffer::applyAsRenderTarget(RenderState& _rs, glm::vec4 _clearColor,
                                      unsigned int _vpWidth, unsigned int _vpHeight) {

    if (!m_glFrameBufferHandle) {
        init(_rs, _vpWidth, _vpHeight);

        m_width = _vpWidth;
        m_height = _vpHeight;
    }

    if (!m_valid) {
        return false;
    }

    _rs.framebuffer(m_glFrameBufferHandle);
    _rs.depthMask(GL_TRUE);
    _rs.viewport(0, 0, _vpWidth, _vpHeight);

    _rs.clearColor(_clearColor.r, _clearColor.g, _clearColor.b, _clearColor.a);

    GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    return true;
}

void FrameBuffer::bind(RenderState& _rs) const {

    if (m_valid) {
        _rs.framebuffer(m_glFrameBufferHandle);
    }
}

void FrameBuffer::init(RenderState& _rs, unsigned int _rtWidth, unsigned int _rtHeight) {

    GL_CHECK(glGenFramebuffers(1, &m_glFrameBufferHandle));

    _rs.framebuffer(m_glFrameBufferHandle);

    // Setup color render target
    if (m_colorRenderBuffer) {
        GL_CHECK(glGenRenderbuffers(1, &m_glColorRenderBufferHandle));
        GL_CHECK(glBindRenderbuffer(GL_RENDERBUFFER, m_glColorRenderBufferHandle));
        GL_CHECK(glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8_OES,
                                       _rtWidth, _rtHeight));

        GL_CHECK(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                           GL_RENDERBUFFER, m_glColorRenderBufferHandle));
    } else {
        TextureOptions options =
            {GL_RGBA, GL_RGBA,
            {GL_NEAREST, GL_NEAREST},
            {GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE}
        };

        m_texture = std::make_unique<Texture>(_rtWidth, _rtHeight, options);
        m_texture->update(_rs, 0);

        GL_CHECK(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                        GL_TEXTURE_2D, m_texture->getGlHandle(), 0));
    }

    {
        // Create depth render buffer
        GL_CHECK(glGenRenderbuffers(1, &m_glDepthRenderBufferHandle));
        GL_CHECK(glBindRenderbuffer(GL_RENDERBUFFER, m_glDepthRenderBufferHandle));
        GL_CHECK(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16,
                                       _rtWidth, _rtHeight));

        GL_CHECK(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                           GL_RENDERBUFFER, m_glDepthRenderBufferHandle));
    }

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
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

            GL_CHECK(glDeleteFramebuffers(1, &glHandle));
        }
    });
}

}
