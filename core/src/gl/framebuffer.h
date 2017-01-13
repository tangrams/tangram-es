#pragma once

#include <memory>
#include <vector>

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

    GLuint readAt(float _normalizedX, float _normalizedY) const;

    struct PixelRect {
        std::vector<GLuint> pixels;
        int32_t left = 0, bottom = 0, width = 0, height = 0;
    };

    PixelRect readRect(float _normalizedX, float _normalizedY, float _normalizedW, float _normalizedH) const;

    void drawDebug(RenderState& _rs, glm::vec2 _dim);

private:

    void init(RenderState& _rs);

    std::unique_ptr<Texture> m_texture;

    Disposer m_disposer;

    GLuint m_glFrameBufferHandle;

    GLuint m_glDepthRenderBufferHandle;

    GLuint m_glColorRenderBufferHandle;

    bool m_valid;

    bool m_colorRenderBuffer;

    int m_width;

    int m_height;

};

}
