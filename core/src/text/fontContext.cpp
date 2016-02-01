#include "fontContext.h"
#define FONTSTASH_IMPLEMENTATION
#include "fontstash.h"

#include "platform.h"
#include "gl/hardware.h"

namespace Tangram {

#define INVALID_FONT -2
#define ATLAS_SIZE 512

FontContext::FontContext() : FontContext(ATLAS_SIZE) {}

FontContext::FontContext(int _atlasSize) {
    initFontContext(_atlasSize);
}

FontContext::~FontContext() {
    fonsDeleteInternal(m_fsContext);
}

void FontContext::bindAtlas(GLuint _textureUnit) {
    {
        std::lock_guard<std::mutex> lock(m_atlasMutex);
        int width, height;
        auto* tex = fonsGetTextureData(m_fsContext, &width, &height);
        m_atlas->update(_textureUnit, reinterpret_cast<const GLuint*>(tex));

        // use size of bound texture for drawing, since
        // atlas size can change on tile-worker thread
        m_boundAtlasSize = { m_atlas->getWidth(), m_atlas->getHeight() };
    }
    m_atlas->bind(_textureUnit);
}

void FontContext::clearState() {
    fonsClearState(m_fsContext);
}

bool FontContext::lock() {
    try {
        m_contextMutex.lock();
    } catch (std::system_error& e) {
        LOGW("Dead lock: aborting");
        return false;
    }
    return true;
}

void FontContext::unlock() {
    m_contextMutex.unlock();
}

FontID FontContext::addFont(const std::string& _family, const std::string& _weight,
                            const std::string& _style, bool _tryFallback) {

    unsigned int dataSize = 0;
    unsigned char* data = nullptr;
    int font = FONS_INVALID;

    std::string fontKey = _family + "_" + _weight + "_" + _style;

    auto it = m_fonts.find(fontKey);
    if (it != m_fonts.end()) {
        if (it->second < 0) {
            goto fallback;
        }
        return it->second;
    }

    {
        // Assuming bundled ttf file follows this convention
        auto bundledFontPath = "fonts/" + _family + "-" + _weight + _style + ".ttf";
        if (!(data = bytesFromFile(bundledFontPath.c_str(), PathType::resource, &dataSize)) &&
            !(data = bytesFromFile(bundledFontPath.c_str(), PathType::internal, &dataSize))) {
            const std::string sysFontPath = systemFontPath(_family, _weight, _style);
            if ( !(data = bytesFromFile(sysFontPath.c_str(), PathType::absolute, &dataSize)) ) {

                LOGE("Could not load font file %s", fontKey.c_str());
                m_fonts.emplace(std::move(fontKey), INVALID_FONT);
                goto fallback;
            }
        }
    }

    font = fonsAddFont(m_fsContext, fontKey.c_str(), data, dataSize);

    if (font == FONS_INVALID) {
        LOGE("Could not load font %s", fontKey.c_str());
        m_fonts.emplace(std::move(fontKey), INVALID_FONT);
        goto fallback;
    }

    m_fonts.emplace(std::move(fontKey), font);

    return font;

fallback:
    if (_tryFallback && m_fonts.size() > 0) {
        return 0;
    }
    return INVALID_FONT;
}

FontContext::FontMetrics FontContext::getMetrics() {
    FontMetrics metrics;
    fonsVertMetrics(m_fsContext, &metrics.ascender, &metrics.descender, &metrics.lineHeight);
    return metrics;
}

FontID FontContext::getFontID(const std::string& _key) {
    auto it = m_fonts.find(_key);

    if (it != m_fonts.end()) {
        return it->second;
    }

    if (m_fonts.size() > 0) {
        // sceneLoader makes sure that first loaded font is the default bundled font
        LOGW("Using default font for '%s'.", _key.c_str());
        m_fonts.emplace(_key, 0);
        return 0;
    } else {
        LOGE("No default font loaded.");
        return -1;
    }
}

std::vector<FONSquad>& FontContext::rasterize(const std::string& _text, FontID _fontID,
                                              float _fontSize, float _sdf) {

    m_quadBuffer.clear();

    if (fonsTextDrawable(m_fsContext, _text.c_str(), _text.c_str() + _text.length(), 1)) {
        fonsSetSize(m_fsContext, _fontSize);
        fonsSetFont(m_fsContext, _fontID);

        if (_sdf > 0){
            fonsSetBlur(m_fsContext, _sdf);
            fonsSetBlurType(m_fsContext, FONS_EFFECT_DISTANCE_FIELD);
        } else {
            fonsSetBlurType(m_fsContext, FONS_EFFECT_NONE);
        }

        float advance = fonsDrawText(m_fsContext, 0, 0,
                _text.c_str(), _text.c_str() + _text.length(),
                0);
        if (advance < 0) {
            m_quadBuffer.clear();
            return m_quadBuffer;
        }
    } else {
        LOGW("Can't draw text %s", _text.c_str());
    }

    return m_quadBuffer;
}

void FontContext::pushQuad(void* _userPtr, const FONSquad* _quad) {
    FontContext* fontContext = static_cast<FontContext*>(_userPtr);
    fontContext->m_quadBuffer.emplace_back(*_quad);
}

int FontContext::renderCreate(void* _userPtr, int _width, int _height) {
    return 1;
}

void FontContext::renderUpdate(void* _userPtr, int* _rect, const unsigned char* _data) {
    FontContext* fontContext = static_cast<FontContext*>(_userPtr);

    // DONT: deadlock here when the lock is held by FONS_ATLAS_FULL
    if (fontContext->m_handleAtlasFull) { return; }

    uint32_t yoff = _rect[1];
    uint32_t height = _rect[3] - _rect[1];

    std::lock_guard<std::mutex> lock(fontContext->m_atlasMutex);
    fontContext->m_atlas->setDirty(yoff, height);
}

void FontContext::fontstashError(void* _uptr, int _error, int _val) {
    FontContext* fontContext = static_cast<FontContext*>(_uptr);

    switch(_error) {
    case FONS_ATLAS_FULL: {
        // callback during rasterize ()
        fontContext->m_handleAtlasFull = true;
        const auto& tex = fontContext->m_atlas;
        unsigned int nw = tex->getWidth() * 2;
        unsigned int nh = tex->getHeight() * 2;

        if (nw > Hardware::maxTextureSize || nh > Hardware::maxTextureSize) {
            LOGE("Full font texture atlas size reached!");
        } else {
            std::lock_guard<std::mutex> lock(fontContext->m_atlasMutex);

            if (fonsExpandAtlas(fontContext->m_fsContext, nw, nh, 1)) {
                tex->resize(nw, nh);
                LOGW("Texture Atlas resized to %d %d", nw, nh);
            } else {
                LOGE("Unexpected error while expanding the font atlas");
            }
        }
        fontContext->m_handleAtlasFull = false;
        break;
    }
    case FONS_SCRATCH_FULL:
    case FONS_STATES_OVERFLOW:
    case FONS_STATES_UNDERFLOW:
    default:
        LOGE("Unexpected error in Fontstash %d:%d!", _error, _val);
        break;
    }
}

void FontContext::initFontContext(int _atlasSize) {
    m_atlas = std::unique_ptr<Texture>(new Texture(_atlasSize, _atlasSize));

    FONSparams params;
    params.width = _atlasSize;
    params.height = _atlasSize;
    params.flags = (unsigned char)FONS_ZERO_TOPLEFT;

    params.renderCreate = renderCreate;
    params.renderResize = nullptr;
    params.renderUpdate = renderUpdate;
    params.renderDraw = nullptr;
    params.renderDelete = nullptr;
    params.pushQuad = pushQuad;
    params.userPtr = (void*) this;

    m_fsContext = fonsCreateInternal(&params);

    fonsSetErrorCallback(m_fsContext, &fontstashError, (void*) this);
}

}
