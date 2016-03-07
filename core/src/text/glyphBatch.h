#pragma once

#include "gl/vertexLayout.h"
#include "gl/texture.h"
#include "text/textMesh.h"
#include <memory>

namespace Tangram {

constexpr int textureSize = 256;
constexpr int maxTextures = 64;

struct GlyphBatch {

    GlyphBatch(std::shared_ptr<VertexLayout> _vertexLayout)
        : texture(textureSize, textureSize) {

        texData.resize(textureSize * textureSize);
        mesh = std::make_unique<TextMesh>(_vertexLayout, GL_TRIANGLES);
    }

    std::vector<unsigned char> texData;
    Texture texture;
    bool dirty;
    std::unique_ptr<TextMesh> mesh;

    size_t refCount = 0;
};

}
