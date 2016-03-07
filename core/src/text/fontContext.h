#pragma once

#include "alfons/alfons.h"
#include "alfons/fontManager.h"
#include "alfons/atlas.h"
#include "alfons/textBatch.h"
#include "alfons/textShaper.h"
#include "alfons/font.h"
#include "alfons/inputSource.h"

#include "text/glyphBatch.h"

#include <bitset>
#include <mutex>

#define FONT_SIZE 24

namespace Tangram {

namespace alf = alfons;

struct FontMetrics {
    float ascender, descender, lineHeight;
};

class AlfonsContext : public alf::TextureCallback {
public:
    AlfonsContext();

    void setVertexLayout(std::shared_ptr<VertexLayout> _vertexLayout);

    // Synchronized on m_mutex on tile-worker threads
    void addTexture(alf::AtlasID id, uint16_t width, uint16_t height) override;

    // Synchronized on m_mutex, called tile-worker threads
    void addGlyph(alf::AtlasID id, uint16_t gx, uint16_t gy, uint16_t gw, uint16_t gh,
                  const unsigned char* src, uint16_t pad) override;

    void releaseAtlas(std::bitset<maxTextures> _refs);

    void lockAtlas(std::bitset<maxTextures> _refs);

    std::vector<GlyphBatch>& batches();

    // TODO: make those private
    std::mutex m_mutex;
    std::array<int, maxTextures> m_atlasRefCount = {{0}};
    std::shared_ptr<alf::Font> m_font;
    alf::GlyphAtlas m_atlas;

private:
    alf::FontManager m_alfons;

    std::vector<unsigned char> m_sdfBuffer;
    std::vector<GlyphBatch> m_batches;
    std::shared_ptr<VertexLayout> m_vertexLayout;
};

}
