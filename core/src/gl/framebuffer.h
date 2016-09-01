#pragma once

#include <memory>
#include <bitset>

#include "gl/disposer.h"
#include "gl.h"

namespace Tangram {

class Texture;
class RenderState;

class FrameBuffer {

public:

    FrameBuffer(bool _colorRenderBuffer = true);

    ~FrameBuffer();

    void applyAsRenderTarget(RenderState& _rs,
                             unsigned int _rtWidth, unsigned int _rtHeight,
                             unsigned int _vpWidth, unsigned int _vpHeight);

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

};

}
