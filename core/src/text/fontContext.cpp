#include "fontContext.h"

#include "platform.h"

#define SDF_IMPLEMENTATION
#include "sdf.h"

struct FallbackPath {
    PathType type;
    std::string path;
};

static std::vector<FallbackPath> defaultFontFallbacks = {
    {PathType::resource, "fonts/NotoSans-Regular.ttf"},
    {PathType::resource, "fonts/NotoNaskh-Regular.ttf"},
    {PathType::resource, "fonts/NotoSansHebrew-Regular.ttf"},
    {PathType::resource, "fonts/DroidSansJapanese.ttf"},
    {PathType::resource, "fonts/DroidSansFallback.ttf"}
};

#define BASE_SIZE           16  // Default font base size, size at which the font would get rasterized
                                // When a glyph get rasterized the closest size is picked, then texture
                                // scaling is applied to it (using its distance field)
#define STEP_SIZE           12  // The step size increase applied, which means that each font exists
                                // at a specific text size within the font renderer
#define MAX_STEPS           3   // The maximum number of steps to increase the size to
#define DEFAULT_BOLDNESS    400
#define SDF_WIDTH           6
#define MIN_LINE_WIDTH      4

namespace Tangram {

bool addFace(std::shared_ptr<alfons::Font>& font,
    alfons::FontManager& manager,
    const FallbackPath& fallback, float size)
{
    alfons::InputSource source;
    unsigned int byteSize;
    unsigned char* data;

    data = bytesFromFile(fallback.path.c_str(), fallback.type, &byteSize);

    if (!data) {
        return false;
    }

    source = alfons::InputSource(reinterpret_cast<char*>(data), byteSize);

    if (!font) {
        font = manager.addFont("default", source, size);
    } else {
        font->addFace(manager.addFontFace(source, size));
    }

    return true;
}

FontContext::FontContext() :
    m_sdfRadius(SDF_WIDTH),
    m_atlas(*this, GlyphTexture::size, m_sdfRadius),
    m_batch(m_atlas, m_scratch)
{
    std::vector<FallbackPath> fallbacks;
    int importance = 0;

    fallbacks.push_back({
        PathType::resource,
        systemFontPath("sans-serif", std::to_string(DEFAULT_BOLDNESS), "normal")
    });

    std::string platformFallback = systemFontFallbackPath(importance++, DEFAULT_BOLDNESS);

    if (!platformFallback.empty()) {
        while (!platformFallback.empty()) {
            fallbacks.push_back({PathType::absolute, platformFallback});
            platformFallback = systemFontFallbackPath(importance++, DEFAULT_BOLDNESS);
        }
    } else {
        // No platform fallback, add bundled font fallbacks
        fallbacks.insert(fallbacks.end(),
            defaultFontFallbacks.begin(),
            defaultFontFallbacks.end());
    }

    // Load fonts to font rendering manager
    for (int i = 0, size = BASE_SIZE; i < MAX_STEPS; i++, size += STEP_SIZE) {
        for (const FallbackPath& fallback : defaultFontFallbacks) {
            if (fallback.path.empty()) { continue; }
            if (!addFace(m_font[i], m_alfons, fallback, size)) {
                LOGW("Failed loading face %s", fallback.path.c_str());
            }
        }
    }
}

// Synchronized on m_mutex on tile-worker threads
void FontContext::addTexture(alfons::AtlasID id, uint16_t width, uint16_t height) {
    if (m_textures.size() == max_textures) {
        LOGE("Way too many glyph textures!");
        return;
    }
    m_textures.emplace_back();
}

// Synchronized on m_mutex, called tile-worker threads
void FontContext::addGlyph(alfons::AtlasID id, uint16_t gx, uint16_t gy, uint16_t gw, uint16_t gh,
                           const unsigned char* src, uint16_t pad) {

    if (id >= max_textures) { return; }

    auto& texData = m_textures[id].texData;
    auto& texture = m_textures[id].texture;
    m_textures[id].dirty = true;

    uint16_t stride = GlyphTexture::size;
    uint16_t width =  GlyphTexture::size;

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

    sdfBuildDistanceFieldNoAlloc(dst, width, m_sdfRadius,
                                 dst, gw, gh, width,
                                 &tmpSdfBuffer[0]);

    texture.setDirty(gy, gh);
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

void FontContext::lockAtlas(std::bitset<max_textures> _refs) {
    if (!_refs.any()) { return; }
    std::lock_guard<std::mutex> lock(m_mutex);
    for (size_t i = 0; i < m_textures.size(); i++) {
        if (_refs[i]) { m_atlasRefCount[i]++; }
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
                             std::vector<GlyphQuad>& _quads,  glm::vec2& _size) {

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

    if (_params.wordWrap) {
        auto wrap = drawWithLineWrapping(line, m_batch, MIN_LINE_WIDTH,
                                         _params.maxLineWidth, _params.align,
                                         _params.lineSpacing);
        metrics = wrap.metrics;
    } else {
        glm::vec2 position(0);
        m_batch.drawShapeRange(line, 0, line.shapes().size(), position, metrics);
    }

    // TextLabel parameter: Dimension
    float width = metrics.aabb.z - metrics.aabb.x;
    float height = metrics.aabb.w - metrics.aabb.y;

    // Offset to center all glyphs around 0/0
    glm::vec2 offset((metrics.aabb.x + width * 0.5) * TextVertex::position_scale,
                     (metrics.aabb.y + height * 0.5) * TextVertex::position_scale);

    auto it = _quads.begin() + quadsStart;
    while (it != _quads.end()) {
        it->quad[0].pos -= offset;
        it->quad[1].pos -= offset;
        it->quad[2].pos -= offset;
        it->quad[3].pos -= offset;
        ++it;
    }

    _size = glm::vec2(width, height);

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

    unsigned int dataSize = 0;
    unsigned char* data = nullptr;

    // Assuming bundled ttf file follows this convention
    auto bundledFontPath = "fonts/" + _family + "-" + _weight + _style + ".ttf";
    if (!(data = bytesFromFile(bundledFontPath.c_str(), PathType::resource, &dataSize)) &&
        !(data = bytesFromFile(bundledFontPath.c_str(), PathType::internal, &dataSize))) {
        const std::string sysFontPath = systemFontPath(_family, _weight, _style);
        if (!(data = bytesFromFile(sysFontPath.c_str(), PathType::absolute, &dataSize))) {

            LOGE("Could not load font file %s", fontName.c_str());
            // add fallbacks from default font
            font->addFaces(*m_font[sizeIndex]);
            return font;
        }
    }

    font->addFace(m_alfons.addFontFace(alfons::InputSource(reinterpret_cast<char*>(data), dataSize), fontSize));

    // add fallbacks from default font
    font->addFaces(*m_font[sizeIndex]);

    return font;
}


}
