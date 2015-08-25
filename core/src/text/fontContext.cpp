#include "fontContext.h"
#define FONTSTASH_IMPLEMENTATION
#include "fontstash.h"

namespace Tangram {

FontContext::FontContext() : FontContext(512) {}

FontContext::FontContext(int _atlasSize) {
    initFontContext(_atlasSize);
}

FontContext::~FontContext() {
    fonsDeleteInternal(m_fsContext);
}

void FontContext::bindAtlas(GLuint _textureUnit) {
    {
        std::lock_guard<std::mutex> lock(m_atlasMutex);
        m_atlas->update(_textureUnit);
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
        logMsg("Dead lock: aborting\n");
        return false;
    }
    return true;
}

void FontContext::unlock() {
    m_contextMutex.unlock();
}

bool FontContext::addFont(const std::string& _name, const std::string& _weight, const std::string& _face) {

    std::string bundledFontName = _name + "-" + _weight + _face;
    std::string systemFontName = constructFontFilename(_name, _weight, _face);
    if (m_fonts.find(bundledFontName) != m_fonts.end() || m_fonts.find(systemFontName) != m_fonts.end()) {
        return true;
    }

    unsigned int dataSize;
    unsigned char* data;

    auto& fontName = bundledFontName;
    auto fontFile = fontName + ".ttf";

    if ( !(data = bytesFromResource(fontFile.c_str(), &dataSize))) {

        fontName = systemFontName;
        fontFile = fontName + ".ttf";
        std::string deviceFontFile = deviceFontsPath() + fontFile;
        if ( !(data = bytesFromExtMemory(deviceFontFile.c_str(), &dataSize)) ) {
            return false;
        }
    }

    int font = fonsAddFont(m_fsContext, fontName.c_str(), data, dataSize);

    if (font == FONS_INVALID) {
        logMsg("[FontContext] Error loading font file %s\n", fontFile.c_str());
        return false;
    }

    m_fonts.emplace(std::move(fontName), font);

    return true;
}

void FontContext::setFont(const std::string& _name, int size) {
    auto it = m_fonts.find(_name);

    if (it != m_fonts.end()) {
        fonsSetSize(m_fsContext, size);
        fonsSetFont(m_fsContext, it->second);
    } else {
        logMsg("[FontContext] Could not find font %s\n", _name.c_str());
    }
}

FontID FontContext::getFontID(const std::string& _name) {
    auto it = m_fonts.find(_name);

    if (it != m_fonts.end()) {
        return it->second;
    } else {
        logMsg("[FontContext] Could not find font %s\n", _name.c_str());
        return -1;
    }
}

std::vector<FONSquad>& FontContext::rasterize(const std::string& _text, FontID _fontID, float _fontSize, float _sdf) {

    m_quadBuffer.clear();

    fonsSetSize(m_fsContext, _fontSize);
    fonsSetFont(m_fsContext, _fontID);

    if (_sdf > 0){
        fonsSetBlur(m_fsContext, _sdf);
        fonsSetBlurType(m_fsContext, FONS_EFFECT_DISTANCE_FIELD);
    } else {
        fonsSetBlurType(m_fsContext, FONS_EFFECT_NONE);
    }

    float advance = fonsDrawText(m_fsContext, 0, 0, _text.c_str(), nullptr, 0);
    if (advance < 0) {
        m_quadBuffer.clear();
        return m_quadBuffer;
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

    uint32_t xoff = 0;
    uint32_t yoff = _rect[1];
    uint32_t width = fontContext->m_atlas->getWidth();
    uint32_t height = _rect[3] - _rect[1];

    auto subdata = reinterpret_cast<const GLuint*>(_data + yoff * width);

    std::lock_guard<std::mutex> lock(fontContext->m_atlasMutex);
    fontContext->m_atlas->setSubData(subdata, xoff, yoff, width, height);
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

    m_fsContext = fonsCreateInternal(&params);;
}

}
