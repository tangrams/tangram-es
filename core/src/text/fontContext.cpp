#include "fontContext.h"

#include "platform.h"

#define SDF_IMPLEMENTATION
#include "sdf.h"

#include <memory>

#define DEFAULT "fonts/NotoSans-Regular.ttf"
#define FONT_AR "fonts/NotoNaskh-Regular.ttf"
#define FONT_HE "fonts/NotoSansHebrew-Regular.ttf"
#define FONT_JA "fonts/DroidSansJapanese.ttf"
#define FALLBACK "fonts/DroidSansFallback.ttf"

#if defined(PLATFORM_ANDROID)
#define ANDROID_FONT_PATH "/system/fonts/"
#endif
#define BASE_SIZE 16
#define STEP_SIZE 12

#define SDF_WIDTH 6

#define MIN_LINE_WIDTH 4

namespace Tangram {

FontContext::FontContext() :
    m_sdfRadius(SDF_WIDTH),
    m_atlas(*this, GlyphTexture::size, m_sdfRadius),
    m_batch(m_atlas, m_scratch),
    m_sceneResourceRoot("") {

// TODO: make this platform independent
#if defined(PLATFORM_ANDROID)
    auto fontPath = systemFontPath("sans-serif", "400", "normal");
    LOGD("FONT %s", fontPath.c_str());

    int size = BASE_SIZE;
    for (int i = 0; i < 3; i++, size += STEP_SIZE) {
        m_font[i] = m_alfons.addFont("default", alfons::InputSource(fontPath), size);
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
        LOGD("FALLBACK %s", fontPath.c_str());

        int size = BASE_SIZE;
        for (int i = 0; i < 3; i++, size += STEP_SIZE) {
            m_font[i]->addFace(m_alfons.addFontFace(alfons::InputSource(fontPath), size));
        }
    }
#elif defined(PLATFORM_IOS)

    int size = BASE_SIZE;
    auto loadFonts = [&](const char* path) {
        size_t dataSize;
        char* data = reinterpret_cast<char*>(bytesFromFile(path, dataSize));
        if (data) {
            for (int i = 0; i < 3; i++, size += STEP_SIZE) {
                m_font[i] = m_alfons.addFont("default", alfons::InputSource(data, dataSize), size);
            }
            free(data);
        }
    };
    auto addFaces = [&](const char* path) {
        size_t dataSize;
        char* data = reinterpret_cast<char*>(bytesFromFile(path, dataSize));
        if (data) {
            for (int i = 0; i < 3; i++, size += STEP_SIZE) {
                    m_font[i]->addFace(m_alfons.addFontFace(alfons::InputSource(data, dataSize), size));
            }
            free(data);
        }
    };

    loadFonts(DEFAULT);
    addFaces(FONT_AR);
    addFaces(FONT_HE);
    addFaces(FONT_JA);
    addFaces(FALLBACK);

#else
    int size = BASE_SIZE;
    for (int i = 0; i < 3; i++, size += STEP_SIZE) {
        m_font[i] = m_alfons.addFont("default", alfons::InputSource(DEFAULT), size);
        m_font[i]->addFace(m_alfons.addFontFace(alfons::InputSource(FONT_AR), size));
        m_font[i]->addFace(m_alfons.addFontFace(alfons::InputSource(FONT_HE), size));
        m_font[i]->addFace(m_alfons.addFontFace(alfons::InputSource(FONT_JA), size));
        m_font[i]->addFace(m_alfons.addFontFace(alfons::InputSource(FALLBACK), size));
    }
#endif
}

// Synchronized on m_mutex in layoutText(), called on tile-worker threads
void FontContext::addTexture(alfons::AtlasID id, uint16_t width, uint16_t height) {
    if (m_textures.size() == max_textures) {
        LOGE("Way too many glyph textures!");
        return;
    }
    m_textures.emplace_back();
}

// Synchronized on m_mutex in layoutText(), called on tile-worker threads
void FontContext::addGlyph(alfons::AtlasID id, uint16_t gx, uint16_t gy, uint16_t gw, uint16_t gh,
                           const unsigned char* src, uint16_t pad) {

    if (id >= max_textures) { return; }

    auto& texData = m_textures[id].texData;
    auto& texture = m_textures[id].texture;

    size_t stride = GlyphTexture::size;
    size_t width =  GlyphTexture::size;

    unsigned char* dst = &texData[(gx + pad) + (gy + pad) * stride];

    for (size_t y = 0, pos = 0; y < gh; y++, pos += gw) {
        std::memcpy(dst + (y * stride), src + pos, gw);
    }

    dst = &texData[size_t(gx) + (size_t(gy) * width)];
    gw += pad * 2;
    gh += pad * 2;

    size_t bytes = size_t(gw) * size_t(gh) * sizeof(float) * 3;
    if (m_sdfBuffer.size() < bytes) {
        m_sdfBuffer.resize(bytes);
    }

    sdfBuildDistanceFieldNoAlloc(dst, width, m_sdfRadius,
                                 dst, gw, gh, width,
                                 &m_sdfBuffer[0]);

    texture.setDirty(gy, gh);
    m_textures[id].dirty = true;
}

void FontContext::releaseAtlas(std::bitset<max_textures> _refs) {
    if (!_refs.any()) { return; }
    std::lock_guard<std::mutex> lock(m_mutex);
    for (size_t i = 0; i < m_textures.size(); i++) {
        if (!_refs[i]) { continue; }

        if (--m_atlasRefCount[i] == 0) {
            LOGD("CLEAR ATLAS %d", i);
            m_atlas.clear(i);
            m_textures[i].texData.assign(GlyphTexture::size * GlyphTexture::size, 0);
        }
    }
}

void FontContext::updateTextures() {
    std::lock_guard<std::mutex> lock(m_mutex);

    for (auto& gt : m_textures) {
        if (gt.dirty || !gt.texture.isValid()) {
            gt.dirty = false;
            auto td = reinterpret_cast<const GLuint*>(gt.texData.data());
            gt.texture.update(0, td);
        }
    }
}

void FontContext::bindTexture(alfons::AtlasID _id, GLuint _unit) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_textures[_id].texture.bind(_unit);

}

bool FontContext::layoutText(TextStyle::Parameters& _params, const std::string& _text,
                             std::vector<GlyphQuad>& _quads, std::bitset<max_textures>& _refs,
                             glm::vec2& _size, TextRange& _textRanges) {

    std::lock_guard<std::mutex> lock(m_mutex);

    alfons::LineLayout line = m_shaper.shape(_params.font, _text);

    if (line.shapes().size() == 0) {
        LOGD("Empty text line");
        return false;
    }

    line.setScale(_params.fontScale);

    // m_batch.drawShapeRange() calls FontContext's TextureCallback for new glyphs
    // and MeshCallback (drawGlyph) for vertex quads of each glyph in LineLayout.

    m_scratch.quads = &_quads;

    size_t quadsStart = _quads.size();
    alfons::LineMetrics metrics;

    std::array<bool, 3> alignments = {};
    if (_params.align != TextLabelProperty::Align::none) {
        alignments[int(_params.align)] = true;
    }

    // Collect possible alignment from anchor fallbacks
    for (int i = 0; i < _params.labelOptions.anchors.count; i++) {
        auto anchor = _params.labelOptions.anchors[i];
        TextLabelProperty::Align alignment = TextLabelProperty::alignFromAnchor(anchor);
        if (alignment != TextLabelProperty::Align::none) {
            alignments[int(alignment)] = true;
        }
    }

    if (_params.wordWrap) {
        m_textWrapper.clearWraps();

        float width = m_textWrapper.getShapeRangeWidth(line, MIN_LINE_WIDTH,
                                                       _params.maxLineWidth);

        for (size_t i = 0; i < 3; i++) {

            int rangeStart = m_scratch.quads->size();
            if (!alignments[i]) {
                _textRanges[i] = Range(rangeStart, 0);
                continue;
            }
            m_textWrapper.draw(m_batch, width, line, TextLabelProperty::Align(i),
                               _params.lineSpacing, metrics);

            int rangeEnd = m_scratch.quads->size();

            _textRanges[i] = Range(rangeStart, rangeEnd - rangeStart);
        }
    } else {
        for (size_t i = 0; i < 3; i++) {
            glm::vec2 position(0);
            int rangeStart = m_scratch.quads->size();
            if (!alignments[i]) {
                _textRanges[i] = Range(rangeStart, 0);
                continue;
            }
            m_batch.drawShapeRange(line, 0, line.shapes().size(), position, metrics);
            int rangeEnd = m_scratch.quads->size();

            _textRanges[i] = Range(rangeStart, rangeEnd - rangeStart);
        }
    }

    auto it = _quads.begin() + quadsStart;
    if (it == _quads.end()) {
        // No glyphs added
        return false;
    }

    // TextLabel parameter: Dimension
    float width = metrics.aabb.z - metrics.aabb.x;
    float height = metrics.aabb.w - metrics.aabb.y;
    _size = glm::vec2(width, height);

    // Offset to center all glyphs around 0/0
    glm::vec2 offset((metrics.aabb.x + width * 0.5) * TextVertex::position_scale,
                     (metrics.aabb.y + height * 0.5) * TextVertex::position_scale);


    for (; it != _quads.end(); ++it) {

        if (!_refs[it->atlas]) {
            _refs[it->atlas] = true;
            m_atlasRefCount[it->atlas]++;
        }

        it->quad[0].pos -= offset;
        it->quad[1].pos -= offset;
        it->quad[2].pos -= offset;
        it->quad[3].pos -= offset;
    }

    return true;
}

void FontContext::ScratchBuffer::drawGlyph(const alfons::Rect& q, const alfons::AtlasGlyph& atlasGlyph) {
    if (atlasGlyph.atlas >= max_textures) { return; }

    auto& g = *atlasGlyph.glyph;

    quads->push_back({
            atlasGlyph.atlas,
            {{glm::vec2{q.x1, q.y1} * TextVertex::position_scale, {g.u1, g.v1}},
             {glm::vec2{q.x1, q.y2} * TextVertex::position_scale, {g.u1, g.v2}},
             {glm::vec2{q.x2, q.y1} * TextVertex::position_scale, {g.u2, g.v1}},
             {glm::vec2{q.x2, q.y2} * TextVertex::position_scale, {g.u2, g.v2}}}});
}

auto FontContext::getFont(const std::string& _family, const std::string& _style,
                          const std::string& _weight, float _size) -> std::shared_ptr<alfons::Font> {

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

    static std::string fontName;

    fontName.clear();
    fontName += _family;
    fontName += '_';
    fontName += _weight;
    fontName += '_';
    fontName += _style;

    auto font = m_alfons.getFont(fontName, fontSize);
    if (font->hasFaces()) { return font; }

    size_t dataSize = 0;
    unsigned char* data = nullptr;

    // Assuming bundled ttf file follows this convention
    auto bundledFontPath = "fonts/" + _family + "-" + _weight + _style + ".ttf";

    do {
        if (!m_sceneResourceRoot.empty()) {
            auto resourceFontPath = m_sceneResourceRoot + bundledFontPath;
            data = bytesFromFile(resourceFontPath.c_str(), dataSize);
            if (data) { break; }
        }

        data = bytesFromFile(bundledFontPath.c_str(), dataSize);
        if (data) { break; }

        auto sysFontPath = systemFontPath(_family, _weight, _style);
        data = bytesFromFile(sysFontPath.c_str(), dataSize);

    } while(0);

    if (!data) {
        LOGE("Could not load font file %s", fontName.c_str());
        // add fallbacks from default font
        font->addFaces(*m_font[sizeIndex]);
        return font;
    }

    font->addFace(m_alfons.addFontFace(alfons::InputSource(reinterpret_cast<char*>(data), dataSize), fontSize));
    free(data);

    // add fallbacks from default font
    font->addFaces(*m_font[sizeIndex]);

    return font;
}


}
