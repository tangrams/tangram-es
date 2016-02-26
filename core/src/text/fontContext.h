#pragma once

#include "alfons/alfons.h"
#include "alfons/fontManager.h"
#include "alfons/atlas.h"
#include "alfons/textBatch.h"
#include "alfons/textShaper.h"
#include "alfons/font.h"
#include "alfons/inputSource.h"

#include "gl/texture.h"

#include <bitset>
#include <mutex>

namespace Tangram {

constexpr int textureSize = 256;
constexpr int maxTextures = 64;

namespace alf = alfons;

struct FontMetrics {
    float ascender, descender, lineHeight;
};

// TODO could be a shared_ptr<Texture>
struct GlyphBatch {

    GlyphBatch() : texture(textureSize, textureSize) {
        texData.resize(textureSize * textureSize);
    }

    std::vector<unsigned char> texData;
    Texture texture;

    bool dirty;
    size_t refCount = 0;
};

class FontContext : public alf::TextureCallback {
public:
    FontContext();

    // Synchronized on m_mutex on tile-worker threads
    void addTexture(alf::AtlasID id, uint16_t width, uint16_t height) override;

    // Synchronized on m_mutex, called tile-worker threads
    void addGlyph(alf::AtlasID id, uint16_t gx, uint16_t gy, uint16_t gw, uint16_t gh,
                  const unsigned char* src, uint16_t pad) override;

    void releaseAtlas(std::bitset<maxTextures> _refs);

    void lockAtlas(std::bitset<maxTextures> _refs);

    alf::GlyphAtlas& atlas() { return m_atlas; }

    std::mutex& mutex() { return m_mutex; }

    void updateTextures();

    std::shared_ptr<alf::Font> getFont(const std::string& _name, const std::string& _style,
                                       const std::string& _weight, float _size);

    // TODO lock?
    size_t glyphBatchCount() {
        return m_batches.size();
    }

    void bindTexture(alf::AtlasID _id, GLuint _unit);

    float maxStrokeWidth() { return m_sdfRadius; }

private:
    float m_sdfRadius;

    std::mutex m_mutex;
    std::array<int, maxTextures> m_atlasRefCount = {{0}};
    alf::GlyphAtlas m_atlas;

    alf::FontManager m_alfons;
    std::array<std::shared_ptr<alf::Font>, 3> m_font;

    std::vector<GlyphBatch> m_batches;
};

}
