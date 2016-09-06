#pragma once

#include <memory>
#include <bitset>

#include "glm/vec4.hpp"
#include "gl/disposer.h"
#include "gl.h"

namespace Tangram {

class Texture;
class RenderState;

class FrameBuffer {

public:

    FrameBuffer(bool _colorRenderBuffer = true);

    ~FrameBuffer();

    bool applyAsRenderTarget(RenderState& _rs, glm::vec4 _clearColor,
                             unsigned int _vpWidth, unsigned int _vpHeight);

    bool valid() const { return m_valid; }

    int getWidth() const { return m_width; }

    int getHeight() const { return m_height; }

    void bind(RenderState& _rs) const;

private:

    void init(RenderState& _rs, unsigned int _rtWidth, unsigned int _rtHeight);

    std::unique_ptr<Texture> m_texture;

    Disposer m_disposer;

    GLuint m_glFrameBufferHandle;

    GLuint m_glDepthRenderBufferHandle;

    GLuint m_glColorRenderBufferHandle;

    int m_generation;

    bool m_valid;

    bool m_colorRenderBuffer;

    int m_width;

    int m_height;

};

}
