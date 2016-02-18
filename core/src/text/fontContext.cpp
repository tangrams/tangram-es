#include "fontContext.h"

#include "platform.h"

#define SDF_IMPLEMENTATION
#include "sdf.h"

#define DEFAULT "fonts/NotoSans-Regular.ttf"
#define FONT_AR "fonts/NotoNaskh-Regular.ttf"
#define FONT_HE "fonts/NotoSansHebrew-Regular.ttf"
#define FONT_JA "fonts/DroidSansJapanese.ttf"
#define FALLBACK "fonts/DroidSansFallback.ttf"

#define SDF_WIDTH 3

#if defined(PLATFORM_ANDROID)
#define BASE_SIZE 24
#define ANDROID_FONT_PATH "/system/fonts/"
#else
#define BASE_SIZE 16
#endif
#define STEP_SIZE 12

namespace Tangram {

AlfonsContext::AlfonsContext() : m_atlas(*this, textureSize) {
#if defined(PLATFORM_ANDROID)
#define BASE_SIZE 16
    auto fontPath = systemFontPath("sans-serif", "400", "normal");
    LOG("FONT %s", fontPath.c_str());

    int size = BASE_SIZE;
    for (int i = 0; i < 3; i++, size += STEP_SIZE) {
        m_font[i] = m_alfons.addFont("default", alf::InputSource(fontPath), size);
    }

    std::string fallback = "";
    int importance = 0;

    while (importance < 100) {
        fallback = systemFontFallbackPath(importance++, 400);
        if (fallback.empty()) { break; }

        if (fallback.find("UI-") != std::string::npos) {
            continue;
        }
        fontPath = ANDROID_FONT_PATH;
        fontPath += fallback;
        LOG("FALLBACK %s", fontPath.c_str());

        int size = BASE_SIZE;
        for (int i = 0; i < 3; i++, size += STEP_SIZE) {
            m_font[i]->addFace(m_alfons.addFontFace(alf::InputSource(fontPath), size));
        }
    }
#else
    int size = BASE_SIZE;
    for (int i = 0; i < 3; i++, size += STEP_SIZE) {
        m_font[i] = m_alfons.addFont("default", alf::InputSource(DEFAULT), size);
        m_font[i]->addFace(m_alfons.addFontFace(alf::InputSource(FONT_AR), size));
        m_font[i]->addFace(m_alfons.addFontFace(alf::InputSource(FONT_HE), size));
        m_font[i]->addFace(m_alfons.addFontFace(alf::InputSource(FONT_JA), size));
        m_font[i]->addFace(m_alfons.addFontFace(alf::InputSource(FALLBACK), size));
    }
#endif
}

// Synchronized on m_mutex on tile-worker threads
void AlfonsContext::addTexture(alf::AtlasID id, uint16_t width, uint16_t height) {
    m_batches.emplace_back();
}

// Synchronized on m_mutex, called tile-worker threads
void AlfonsContext::addGlyph(alf::AtlasID id, uint16_t gx, uint16_t gy, uint16_t gw, uint16_t gh,
              const unsigned char* src, uint16_t pad)
{
    auto& texData = m_batches[id].texData;
    auto& texture = m_batches[id].texture;
    m_batches[id].dirty = true;

    uint16_t stride = textureSize;
    uint16_t width = textureSize;

    unsigned char* dst = &texData[(gx + pad) + (gy + pad) * stride];

    for (size_t y = 0, pos = 0; y < gh; y++, pos += gw) {
        std::memcpy(dst + y * stride, src + pos, gw);
    }

    dst = &texData[gx + gy * width];
    gw += pad * 2;
    gh += pad * 2;

    static std::vector<unsigned char> tmpSdfBuffer;

    size_t bytes = gw * gh * sizeof(float) * 3;
    if (tmpSdfBuffer.size() < bytes) {
        tmpSdfBuffer.resize(bytes);
    }

    sdfBuildDistanceFieldNoAlloc(dst, width, SDF_WIDTH,
                                 dst, gw, gh, width,
                                 &tmpSdfBuffer[0]);

    texture.setDirty(gy, gh);
}

void AlfonsContext::releaseAtlas(std::bitset<maxTextures> _refs) {
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

void AlfonsContext::lockAtlas(std::bitset<maxTextures> _refs) {
    if (!_refs.any()) { return; }
    std::lock_guard<std::mutex> lock(m_mutex);
    for (size_t i = 0; i < m_batches.size(); i++) {
        if (_refs[i]) { m_atlasRefCount[i]++; }
    }
}

void AlfonsContext::updateTextures() {
    std::lock_guard<std::mutex> lock(m_mutex);

    for (auto& batch : m_batches) {
        if (batch.dirty || !batch.texture.isValid()) {
            batch.dirty = false;
            auto td = reinterpret_cast<const GLuint*>(batch.texData.data());
            batch.texture.update(0, td);
        }
    }
}

void AlfonsContext::bindTexture(alf::AtlasID _id, GLuint _unit) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_batches[_id].texture.bind(_unit);

}

auto AlfonsContext::getFont(const std::string& _family, const std::string& _style,
                            const std::string& _weight, float _size) -> std::shared_ptr<alf::Font> {

    std::string fontName = _family + "_" + _weight + "_" + _style;

    int sizeIndex = 0;

    // Pick the smallest font that does not scale down too much
    float fontSize = BASE_SIZE;
    for (int i = 0; i < 3; i++) {
        sizeIndex = i;

        if (_size <= fontSize) { break; }
        fontSize += STEP_SIZE;
    }
    //LOG(">> %f - %d ==> %f", _size, sizeIndex, _size / ((sizeIndex+1) * BASE_SIZE));

    std::lock_guard<std::mutex> lock(m_mutex);

    auto font = m_alfons.getFont(fontName, fontSize);
    if (font->hasFaces()) { return font; }

    unsigned int dataSize = 0;
    unsigned char* data = nullptr;

    // Assuming bundled ttf file follows this convention
    auto bundledFontPath = "fonts/" + _family + "-" + _weight + _style + ".ttf";
    if (!(data = bytesFromFile(bundledFontPath.c_str(), PathType::resource, &dataSize)) &&
        !(data = bytesFromFile(bundledFontPath.c_str(), PathType::internal, &dataSize))) {
        const std::string sysFontPath = systemFontPath(_family, _weight, _style);
        if (!(data = bytesFromFile(sysFontPath.c_str(), PathType::absolute, &dataSize)) ) {

            LOGE("Could not load font file %s", fontName.c_str());
            // add fallbacks from default font
            font->addFaces(*m_font[sizeIndex]);
            return font;
        }
    }

    font->addFace(m_alfons.addFontFace(alf::InputSource(reinterpret_cast<char*>(data), dataSize), fontSize));

    // add fallbacks from default font
    font->addFaces(*m_font[sizeIndex]);

    return font;
}

}
