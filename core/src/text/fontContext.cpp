#include "fontContext.h"
#define GLFONTSTASH_IMPLEMENTATION
#include "glfontstash.h"

FontContext::FontContext() : FontContext(512) {}

FontContext::FontContext(int _atlasSize) : m_contextMutex(std_patch::make_unique<std::mutex>()) {
    initFontContext(_atlasSize);
}

FontContext::~FontContext() {
    glfonsDelete(m_fsContext);
}

const std::unique_ptr<Texture>& FontContext::getAtlas() const {
    return m_atlas;
}

void FontContext::setScreenSize(int _width, int _height) {
    glfonsScreenSize(m_fsContext, _width, _height);
}

void FontContext::clearState() {
    fonsClearState(m_fsContext);
}

void FontContext::setSignedDistanceField(float _blurSpread) {
    fonsSetBlur(m_fsContext, _blurSpread);
    fonsSetBlurType(m_fsContext, FONS_EFFECT_DISTANCE_FIELD);
}

void FontContext::lock() {
    m_contextMutex->lock();
}

void FontContext::unlock() {
    m_contextMutex->unlock();
}

bool FontContext::addFont(const std::string& _fontFile, std::string _name) {
    if (m_fonts.find(_name) != m_fonts.end()) {
        return true;
    }

    unsigned int dataSize;
    unsigned char* data = bytesFromResource(_fontFile.c_str(), &dataSize);
    int font = fonsAddFont(m_fsContext, "droid-serif", data, dataSize);

    if (font == FONS_INVALID) {
        logMsg("[FontContext] Error loading font file %s\n", _fontFile.c_str());
        return false;
    }

    m_fonts.emplace(std::move(_name), font);

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

void updateAtlas(void* _userPtr, unsigned int _xoff, unsigned int _yoff,
                 unsigned int _width, unsigned int _height, const unsigned int* _pixels) {

    FontContext* fontContext = static_cast<FontContext*>(_userPtr);
    fontContext->getAtlas()->setSubData(static_cast<const GLuint*>(_pixels), _xoff, _yoff, _width, _height);
}

void updateBuffer(void* _userPtr, GLintptr _offset, GLsizei _size, float* _newData, void* _owner) {
    auto buffer = static_cast<TextBuffer*>(_owner);
    
    if (buffer) {
        buffer->update(_offset, _size, reinterpret_cast<unsigned char*>(_newData));
    }
}

void FontContext::initFontContext(int _atlasSize) {
    m_atlas = std::unique_ptr<Texture>(new Texture(_atlasSize, _atlasSize));
    m_fsContext = glfonsCreate(_atlasSize, _atlasSize, FONS_ZERO_TOPLEFT, { false, updateBuffer, updateAtlas }, (void*) this);
}
