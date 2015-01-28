#include "fontContext.h"
#define GLFONTSTASH_IMPLEMENTATION
#include "glfontstash.h"

FontContext::FontContext() : FontContext(512) {}

FontContext::FontContext(int _atlasSize) : m_contextMutex(std_patch::make_unique<std::mutex>()) {
    initFontContext(_atlasSize);
}

FontContext::~FontContext() {
    m_atlas->destroy();
    glfonsDelete(m_fsContext);
}

std::shared_ptr<TextBuffer> FontContext::genTextBuffer() {
    // TODO : remove this, just to retain the shared pointer
    m_buffers.push_back(std::shared_ptr<TextBuffer>(new TextBuffer(m_fsContext)));
    return m_buffers.back();
}

std::shared_ptr<TextBuffer> FontContext::genTextBuffer(int _size) {
    // TODO : remove this, just to retain the shared pointer
    m_buffers.push_back(std::shared_ptr<TextBuffer>(new TextBuffer(m_fsContext, _size)));
    return m_buffers.back();
}

const std::unique_ptr<Texture>& FontContext::getAtlas() const {
    return m_atlas;
}

void FontContext::getViewProjection(float* _projectionMatrix) const {
    glfonsProjection(m_fsContext, _projectionMatrix);
}

void FontContext::setScreenSize(int _width, int _height) {
    glfonsScreenSize(m_fsContext, _width, _height);
}

void FontContext::useBuffer(const std::shared_ptr<TextBuffer>& _textBuffer) {
    m_currentBuffer = _textBuffer;
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

std::shared_ptr<TextBuffer> FontContext::getCurrentBuffer() {
    return m_currentBuffer.lock();
}

bool FontContext::addFont(std::string _fontFile, std::string _name) {
    if (m_fonts.find(_name) != m_fonts.end()) {
        return true;
    }

    unsigned int dataSize;
    unsigned char* data = bytesFromResource(_fontFile.c_str(), &dataSize);
    m_font = fonsAddFont(m_fsContext, "droid-serif", data, dataSize);

    if (m_font == FONS_INVALID) {
        logMsg("[FontContext] Error loading font file %s\n", _fontFile.c_str());
        return false;
    }

    m_fonts[_name] = m_font;

    return true;
}

void FontContext::setFont(std::string _name, int size) {
    auto it = m_fonts.find(_name);

    if (it != m_fonts.end()) {
        fonsSetSize(m_fsContext, size);
        fonsSetFont(m_fsContext, it->second);
    } else {
        logMsg("[FontContext] Could not find font %s\n", _name.c_str());
    }
}

void createTexTransforms(void* _userPtr, unsigned int _width, unsigned int _height) {
    FontContext* fontContext = static_cast<FontContext*>(_userPtr);

    TextureOptions options = {GL_RGBA, GL_RGBA, {GL_LINEAR, GL_LINEAR}, {GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE}};
    std::unique_ptr<Texture> texture(new Texture(_width, _height, 1 /* gpu slot */ , options));
    std::shared_ptr<TextBuffer> textBuffer = fontContext->m_currentBuffer.lock();

    if (textBuffer) {
        textBuffer->setTextureTransform(std::move(texture));
    }
}

void updateTransforms(void* _userPtr, unsigned int _xoff, unsigned int _yoff, unsigned int _width,
                      unsigned int _height, const unsigned int* _pixels, void* _ownerPtr) {

    TextBuffer* buffer = static_cast<TextBuffer*>(_ownerPtr);
    const GLuint* subData = static_cast<const GLuint*>(_pixels);
    const auto& texture = buffer->getTextureTransform();
    texture->setSubData(subData, _xoff, _yoff, _width, _height);
}

void updateAtlas(void* _userPtr, unsigned int _xoff, unsigned int _yoff,
                 unsigned int _width, unsigned int _height, const unsigned int* _pixels) {

    FontContext* fontContext = static_cast<FontContext*>(_userPtr);
    fontContext->m_atlas->setSubData(static_cast<const GLuint*>(_pixels), _xoff, _yoff, _width, _height);
}

void createAtlas(void* _userPtr, unsigned int _width, unsigned int _height) {

    FontContext* fontContext = static_cast<FontContext*>(_userPtr);
    fontContext->m_atlas = std::unique_ptr<Texture>(new Texture(_width, _height));
}

bool errorCallback(void* _userPtr, fsuint buffer, GLFONSError error) {
    FontContext* fontContext = static_cast<FontContext*>(_userPtr);

    bool solved = false;

    switch (error) {
        case GLFONSError::ID_OVERFLOW: {

            // TODO : get texture from currently bound text buffer
            logMsg("[FontStyle] FontError : ID_OVERFLOW in text buffer %d\n", buffer);
            std::shared_ptr<TextBuffer> textBuffer = fontContext->m_currentBuffer.lock();

            if (textBuffer) {
                const auto& texture = textBuffer->getTextureTransform();
                // expand the transform texture in cpu side
                glfonsExpandTransform(fontContext->m_fsContext, buffer, texture->getWidth() * 2);
                // double size of texture
                texture->resize(texture->getWidth() * 2, texture->getHeight() * 2);

                solved = true; // error solved
            }
            
            break;
        }

        default:
            logMsg("[FontContext] FontError : undefined error\n");
            break;
    }

    return solved;
}

void FontContext::initFontContext(int _atlasSize) {
    GLFONSparams params;

    params.errorCallback = errorCallback;
    params.createAtlas = createAtlas;
    params.createTexTransforms = createTexTransforms;
    params.updateAtlas = updateAtlas;
    params.updateTransforms = updateTransforms;

    m_fsContext = glfonsCreate(_atlasSize, _atlasSize, FONS_ZERO_TOPLEFT, params, (void*) this);
}
