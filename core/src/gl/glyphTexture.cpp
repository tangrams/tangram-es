#include "gl/glyphTexture.h"

#include "gl/renderState.h"
#include "log.h"

namespace Tangram {

GlyphTexture::GlyphTexture() : Texture(textureOptions()) {

    m_buffer.reset(reinterpret_cast<GLubyte*>(std::calloc(size * size, sizeof(GLubyte))));
    m_disposeBuffer = false;
    resize(size, size);
}

bool GlyphTexture::bind(RenderState& _rs, GLuint _textureUnit) {

    if (!m_shouldResize && m_dirtyRows.empty()) {
        if (m_glHandle == 0) { return false; }

        _rs.texture(m_glHandle, _textureUnit, GL_TEXTURE_2D);
        return true;
    }

    if (m_shouldResize) {
        m_shouldResize = false;
        m_dirtyRows.clear();
        return upload(_rs, _textureUnit);
    }

    if (m_glHandle == 0) {
        LOGW("Texture is not ready!");
        return false;
    }

    _rs.texture(m_glHandle, _textureUnit, GL_TEXTURE_2D);

    auto format = static_cast<GLenum>(m_options.pixelFormat);
    for (auto& range : m_dirtyRows) {
        auto rows = range.max - range.min;
        auto offset = m_buffer.get() + (range.min * m_width * bpp());
        GL::texSubImage2D(GL_TEXTURE_2D, 0, 0, range.min, m_width, rows, format,
                          GL_UNSIGNED_BYTE, offset);
    }
    m_dirtyRows.clear();
    return true;
}

void GlyphTexture::setRowsDirty(int start, int count) {
    // FIXME: check that dirty range is valid for texture size!
    int max = start + count;
    int min = start;

    if (m_dirtyRows.empty()) {
        m_dirtyRows.push_back({min, max});
        return;
    }

    auto n = m_dirtyRows.begin();

    // Find first overlap
    while (n != m_dirtyRows.end()) {
        if (min > n->max) {
            // this range is after current
            ++n;
            continue;
        }
        if (max < n->min) {
            // this range is before current
            m_dirtyRows.insert(n, {min, max});
            return;
        }
        // Combine with overlapping range
        n->min = std::min(n->min, min);
        n->max = std::max(n->max, max);
        break;
    }
    if (n == m_dirtyRows.end()) {
        m_dirtyRows.push_back({min, max});
        return;
    }

    // Merge up to last overlap
    auto it = n+1;
    while (it != m_dirtyRows.end() && max >= it->min) {
        n->max = std::max(it->max, max);
        it = m_dirtyRows.erase(it);
    }
}
}
