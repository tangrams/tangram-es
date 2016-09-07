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

    FrameBuffer(int _width, int _height, bool _colorRenderBuffer = true);

    ~FrameBuffer();

    bool applyAsRenderTarget(RenderState& _rs, glm::vec4 _clearColor = glm::vec4(0.0));

    static void apply(RenderState& _rs, GLuint _handle, glm::vec2 _viewport, glm::vec4 _clearColor);

    bool valid() const { return m_valid; }

    int getWidth() const { return m_width; }

    int getHeight() const { return m_height; }

    void bind(RenderState& _rs) const;

    GLuint readAt(RenderState& _rs, float _normalizedX, float _normalizedY) const;

private:

    void init(RenderState& _rs);

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
