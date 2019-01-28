#include "text/fontContext.h"

#include "log.h"
#include "platform.h"

#define SDF_IMPLEMENTATION
#include "sdf.h"

#include <memory>
#include <regex>

#define SDF_WIDTH 6

#define MIN_LINE_WIDTH 4

namespace Tangram {

const std::vector<float> FontContext::s_fontRasterSizes = { 16, 28, 40 };

FontContext::FontContext(Platform& _platform) :
    m_sdfRadius(SDF_WIDTH),
    m_atlas(*this, GlyphTexture::size, m_sdfRadius),
    m_batch(m_atlas, m_scratch),
    m_platform(_platform) {}

void FontContext::setPixelScale(float _scale) {
    m_sdfRadius = SDF_WIDTH * _scale;
}

void FontContext::loadFonts() {
    auto fallbacks = m_platform.systemFontFallbacksHandle();

    for (size_t i = 0; i < s_fontRasterSizes.size(); i++) {
        m_font[i] = m_alfons.addFont("default", s_fontRasterSizes[i]);
    }

    bool added = false;
    for (const auto& fallback : fallbacks) {

        if (!fallback.isValid()) {
            LOGD("Invalid fallback font");
            continue;
        }

        alfons::InputSource source;
        switch (fallback.tag) {
            case FontSourceHandle::FontPath:
                source = alfons::InputSource(fallback.fontPath.path());
                break;
            case FontSourceHandle::FontName:
                source = alfons::InputSource(fallback.fontName, true);
                break;
            case FontSourceHandle::FontLoader:
                source = alfons::InputSource(fallback.fontLoader);
                break;
            case FontSourceHandle::None:
            default: {
                LOGD("Invalid fallback font: FontSourceHandle::None");
                continue;
            }
        }
        for (size_t i = 0; i < s_fontRasterSizes.size(); i++) {
            m_font[i]->addFace(m_alfons.addFontFace(source, s_fontRasterSizes[i]));
        }
        added = true;
    }
    if (!added) {
        LOGW("No fallback fonts available!");
    }
}

// Synchronized on m_mutex in layoutText(), called on tile-worker threads
void FontContext::addTexture(alfons::AtlasID id, uint16_t width, uint16_t height) {

    std::lock_guard<std::mutex> lock(m_textureMutex);

    if (m_textures.size() == max_textures) {
        LOGE("Way too many glyph textures!");
        return;
    }
    m_textures.push_back(std::make_unique<GlyphTexture>());
}

// Synchronized on m_mutex in layoutText(), called on tile-worker threads
void FontContext::addGlyph(alfons::AtlasID id, uint16_t gx, uint16_t gy, uint16_t gw, uint16_t gh,
                           const unsigned char* src, uint16_t pad) {

    std::lock_guard<std::mutex> lock(m_textureMutex);

    if (id >= max_textures) { return; }

    auto texData = m_textures[id]->buffer();
    auto& texture = m_textures[id];

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

    texture->setRowsDirty(gy, gh);
}

void FontContext::releaseAtlas(std::bitset<max_textures> _refs) {
    if (!_refs.any()) { return; }
    std::lock_guard<std::mutex> lock(m_textureMutex);

    for (size_t i = 0; i < m_textures.size(); i++) {
        if (_refs[i]) { m_atlasRefCount[i] -= 1; }
    }
}

void FontContext::updateTextures(RenderState& rs) {
    std::lock_guard<std::mutex> lock(m_textureMutex);

    for (auto& gt : m_textures) { gt->bind(rs, 0); }
}

void FontContext::bindTexture(RenderState& rs, alfons::AtlasID _id, GLuint _unit) {
    std::lock_guard<std::mutex> lock(m_textureMutex);

    m_textures[_id]->bind(rs, _unit);
}

bool FontContext::layoutText(TextStyle::Parameters& _params, const icu::UnicodeString& _text,
                             std::vector<GlyphQuad>& _quads, std::bitset<max_textures>& _refs,
                             glm::vec2& _size, TextRange& _textRanges) {

    std::lock_guard<std::mutex> lock(m_fontMutex);

    alfons::LineLayout line = m_shaper.shapeICU(_params.font, _text, MIN_LINE_WIDTH,
                                                _params.wordWrap ? _params.maxLineWidth : 0);

    if (line.missingGlyphs() || line.shapes().size() == 0) {
        // Nothing to do!
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

        if (_params.maxLines != 0) {
            uint32_t numLines = 0;
            int pos = 0;
            int max = line.shapes().size();

            for (auto& shape : line.shapes()) {
                pos++;
                if (shape.mustBreak) {
                    numLines++;
                    if (numLines >= _params.maxLines && pos < max) {
                        shape.mustBreak = false;
                        line.removeShapes(shape.isSpace ? pos-1 : pos, max);

                        auto ellipsis = m_shaper.shape(_params.font, "…");
                        line.addShapes(ellipsis.shapes());
                        break;
                    }
                }
            }
        }

        float width = m_textWrapper.getShapeRangeWidth(line);

        for (size_t i = 0; i < 3; i++) {

            int rangeStart = m_scratch.quads->size();
            if (!alignments[i]) {
                _textRanges[i] = Range(rangeStart, 0);
                continue;
            }
            int numLines = m_textWrapper.draw(m_batch, width, line, TextLabelProperty::Align(i),
                                              _params.lineSpacing, metrics);
            int rangeEnd = m_scratch.quads->size();

            _textRanges[i] = Range(rangeStart, rangeEnd - rangeStart);

            // For single line text alignments are the same
            if (i == 0 && numLines == 1) {
                _textRanges[1] = Range(rangeEnd, 0);
                _textRanges[2] = Range(rangeEnd, 0);
                break;
            }
        }
    } else {
        glm::vec2 position(0);
        int rangeStart = m_scratch.quads->size();
        m_batch.drawShapeRange(line, 0, line.shapes().size(), position, metrics);
        int rangeEnd = m_scratch.quads->size();

        _textRanges[0] = Range(rangeStart, rangeEnd - rangeStart);

        _textRanges[1] = Range(rangeEnd, 0);
        _textRanges[2] = Range(rangeEnd, 0);
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

    {
        std::lock_guard<std::mutex> lock(m_textureMutex);
        for (; it != _quads.end(); ++it) {

            if (!_refs[it->atlas]) {
                _refs[it->atlas] = true;
                m_atlasRefCount[it->atlas] += 1;
            }

            it->quad[0].pos -= offset;
            it->quad[1].pos -= offset;
            it->quad[2].pos -= offset;
            it->quad[3].pos -= offset;
        }

        // Clear unused textures
        for (size_t i = 0; i < m_textures.size(); i++) {
            if (m_atlasRefCount[i] == 0) {
                m_atlas.clear(i);
                std::memset(m_textures[i]->buffer(), 0, GlyphTexture::size * GlyphTexture::size);
            }
        }
    }

    return true;
}

void FontContext::addFont(const FontDescription& _ft, alfons::InputSource _source) {

    // NB: Synchronize for calls from download thread
    std::lock_guard<std::mutex> lock(m_fontMutex);

    for (size_t i = 0; i < s_fontRasterSizes.size(); i++) {
        if (auto font = m_alfons.getFont(_ft.alias, s_fontRasterSizes[i])) {

            font->addFace(m_alfons.addFontFace(_source, s_fontRasterSizes[i]));

            // add fallbacks from default font
            if (m_font[i]) { font->addFaces(*m_font[i]); }
        }
    }
}

void FontContext::releaseFonts() {

    std::lock_guard<std::mutex> lock(m_fontMutex);
    // Unload Freetype and Harfbuzz resources for all font faces
    m_alfons.unload();
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

std::shared_ptr<alfons::Font> FontContext::getFont(const std::string& _family, const std::string& _style,
                                                   const std::string& _weight, float _size) {

    // Pick the smallest font that does not scale down too much
    float fontSize = s_fontRasterSizes.back();
    size_t sizeIndex = s_fontRasterSizes.size() - 1;

    auto fontSizeItr = std::lower_bound(s_fontRasterSizes.begin(), s_fontRasterSizes.end(), _size);
    if (fontSizeItr != s_fontRasterSizes.end()) {
        fontSize = *fontSizeItr;
        sizeIndex = fontSizeItr - s_fontRasterSizes.begin();
    }

    std::lock_guard<std::mutex> lock(m_fontMutex);

    auto font = m_alfons.getFont(FontDescription::Alias(_family, _style, _weight), fontSize);
    if (font->hasFaces()) { return font; }

    // First, try to load from the system fonts.
    bool useFallbackFont = false;
    auto systemFontHandle = m_platform.systemFont(_family, _weight, _style);

    alfons::InputSource source;

    switch (systemFontHandle.tag) {
        case FontSourceHandle::FontPath:
            source = alfons::InputSource(systemFontHandle.fontPath.path());
            break;
        case FontSourceHandle::FontName:
            source = alfons::InputSource(systemFontHandle.fontName, true);
            break;
        case FontSourceHandle::FontLoader:
        {
            auto& loader = systemFontHandle.fontLoader;
            auto fontData = loader();
            if (fontData.size() > 0) {
                source = alfons::InputSource(loader);
            } else {
                useFallbackFont = true;
            }
            break;
        }
        case FontSourceHandle::None:
        default:
            useFallbackFont = true;
    }

    if (!useFallbackFont) {
        font->addFace(m_alfons.addFontFace(source, fontSize));
    } else {
        LOGD("Using fallback font for Family: %s, Style: %s, Weight: %s, Size %f",
             _family.c_str(), _style.c_str(), _weight.c_str(), _size);
    }
    // Add fallbacks from default font.
    if (m_font[sizeIndex]) {
        font->addFaces(*m_font[sizeIndex]);
    }

    return font;
}

}
