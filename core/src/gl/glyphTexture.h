#pragma once

#include "gl/texture.h"

namespace Tangram {

class GlyphTexture : public Texture {
    static constexpr TextureOptions textureOptions() {
        TextureOptions options;
        options.pixelFormat = PixelFormat::ALPHA;
        return options;
    }
public:
    static constexpr int size = 256;

    GlyphTexture();

    bool bind(RenderState& rs, GLuint _unit) override;

    void setRowsDirty(int start, int count);

    GLubyte* buffer() { return m_buffer.get(); }

protected:
    struct DirtyRowRange {
        int min;
        int max;
    };

    std::vector<DirtyRowRange> m_dirtyRows;
};

}
