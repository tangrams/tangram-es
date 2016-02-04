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

#define SDF_IMPLEMENTATION
#include "sdf.h"

// #define DEFAULT_FONT "fonts/amiri-400regular.ttf"
// #define DEFAULT_FONT "/usr/share/fonts/TTF/DejaVuSans.ttf"
// #define DEFAULT_FONT "fonts/open sans-300normal.ttf"
// #define FALLBACK_FONT "fonts/roboto-regular.ttf"

#define DEFAULT "fonts/NotoSans-Regular.ttf"
#define FONT_AR "fonts/NotoNaskh-Regular.ttf"
#define FONT_HE "fonts/NotoSansHebrew-Regular.ttf"
#define FONT_JA "fonts/DroidSansJapanese.ttf"
#define FALLBACK "fonts/DroidSansFallback.ttf"

#define FONT_SIZE 24
#define SDF_WIDTH 3

#define ANDROID_FONT_PATH "/system/fonts/"

namespace Tangram {

namespace alf = alfons;

struct FontMetrics {
    float ascender, descender, lineHeight;
};

class AlfonsContext : public alf::TextureCallback {
public:
    AlfonsContext() :
        m_atlas(*this, textureSize) {

#if defined(PLATFORM_ANDROID)

        auto fontPath = systemFontPath("sans-serif", "400", "normal");
        LOG("FONT %s", fontPath.c_str());

        m_font = m_alfons.addFont(fontPath, FONT_SIZE);

        std::string fallback = "";
        int importance = 0;

        while (importance < 100) {
            fallback = systemFontFallbackPath(importance++, 400);
            if (fallback.empty()) {
                break;
            }
            if (fallback.find("UI-") != std::string::npos) {
                continue;
            }
            fontPath = ANDROID_FONT_PATH;
            fontPath += fallback;
            LOG("FALLBACK %s", fontPath.c_str());

            m_font->addFace(m_alfons.getFontFace(alf::InputSource(fontPath), FONT_SIZE));
        }
#else
        m_font = m_alfons.addFont(DEFAULT, FONT_SIZE);
        m_font->addFace(m_alfons.getFontFace(alf::InputSource(FONT_AR), FONT_SIZE));
        m_font->addFace(m_alfons.getFontFace(alf::InputSource(FONT_HE), FONT_SIZE));
        m_font->addFace(m_alfons.getFontFace(alf::InputSource(FONT_JA), FONT_SIZE));
        m_font->addFace(m_alfons.getFontFace(alf::InputSource(FALLBACK), FONT_SIZE));
#endif
    }

    void setVertexLayout(std::shared_ptr<VertexLayout> _vertexLayout) { m_vertexLayout = _vertexLayout; }

    // Synchronized on m_mutex on tile-worker threads
    void addTexture(alf::AtlasID id, uint16_t width, uint16_t height) override {
        m_batches.emplace_back(m_vertexLayout);
    }

    // Synchronized on m_mutex, called tile-worker threads
    void addGlyph(alf::AtlasID id, uint16_t gx, uint16_t gy, uint16_t gw, uint16_t gh,
                  const unsigned char* src, uint16_t pad) override {

        auto& texData = m_batches[id].texData;
        auto& texture = m_batches[id].texture;
        m_batches[id].dirty = true;

        uint16_t stride = textureSize;
        uint16_t width = textureSize;

        unsigned char* dst = &texData[(gx + pad) + (gy + pad) * stride];

        size_t pos = 0;
        for (uint16_t y = 0; y < gh; y++) {
            size_t row = (y * stride);
            for (uint16_t x = 0; x < gw; x++) {
                dst[row + x] = src[pos++];
            }
        }

        dst = &texData[gx + gy * width];
        gw += pad * 2;
        gh += pad * 2;

        size_t bytes = gw * gh * sizeof(float) * 3;
        if (m_sdfBuffer.size() < bytes) {
            m_sdfBuffer.resize(bytes);
        }

        sdfBuildDistanceFieldNoAlloc(dst, width, SDF_WIDTH,
                                     dst, gw, gh, width,
                                     &m_sdfBuffer[0]);

        texture.setDirty(gy, gh);
    }

    void releaseAtlas(std::bitset<maxTextures> _refs) {
        if (!_refs.any()) { return; }
        std::lock_guard<std::mutex> lock(m_mutex);
        for (size_t i = 0; i < m_batches.size(); i++) {
            if (!_refs[i]) { continue; }

            if (--m_atlasRefCount[i] == 0) {
                LOG("CLEAR ATLAS %d", i);
                m_atlas.clear(i);
                m_batches[i].texData.assign(textureSize * textureSize, 0);
            }
        }
    }

    void lockAtlas(std::bitset<maxTextures> _refs) {
        if (!_refs.any()) { return; }
        std::lock_guard<std::mutex> lock(m_mutex);
        for (size_t i = 0; i < m_batches.size(); i++) {
            if (_refs[i]) { m_atlasRefCount[i]++; }
        }
    }

    std::vector<GlyphBatch>& batches() { return m_batches; }

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
