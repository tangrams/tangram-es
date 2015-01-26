#include "fontContext.h"
#define GLFONTSTASH_IMPLEMENTATION
#include "glfontstash.h"

FontContext::FontContext(std::string _fontFile) : FontContext(_fontFile, 512) {}

FontContext::FontContext(std::string _fontFile, int _atlasSize) {
    initFontContext(_fontFile, _atlasSize);
}

FontContext::~FontContext() {
    m_atlas->destroy();
    glfonsDelete(m_fsContext);
}

std::shared_ptr<TextBuffer> FontContext::genTextBuffer() const {
    return std::shared_ptr<TextBuffer>(new TextBuffer(m_fsContext));
}

std::shared_ptr<TextBuffer> FontContext::genTextBuffer(int _size) const {
    return std::shared_ptr<TextBuffer>(new TextBuffer(m_fsContext, _size));
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

void createTexTransforms(void* _userPtr, unsigned int _width, unsigned int _height) {
    FontContext* fontContext = static_cast<FontContext*>(_userPtr);

    TextureOptions options = {GL_RGBA, GL_RGBA, {GL_LINEAR, GL_LINEAR}, {GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE}};

    std::unique_ptr<Texture> texture(new Texture(_width, _height, 1 /* gpu slot */ , options));

    // TODO : add texture to currently bound text buffer
}

void updateTransforms(void* _userPtr, unsigned int _xoff, unsigned int _yoff, unsigned int _width,
                      unsigned int _height, const unsigned int* _pixels, void* _ownerPtr) {

    FontContext* fontContext = static_cast<FontContext*>(_userPtr);
    TextBuffer* buffer = static_cast<TextBuffer*>(_ownerPtr);

    const GLuint* subData = static_cast<const GLuint*>(_pixels);

    // TODO : get the texture from currently bound text buffer
    // const std::unique_ptr<Texture>& texture = buffer->getTextureTransform();
    // texture->setSubData(subData, _xoff, _yoff, _width, _height);
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
            // logMsg("[FontStyle] FontError : ID_OVERFLOW in text buffer %d\n", buffer);

            // const std::unique_ptr<Texture>& texture = fontStyle->m_processedTile->getTextureTransform(*fontStyle);

            // if (texture) {

            //     // expand the transform texture in cpu side
            //     glfonsExpandTransform(fontStyle->m_fontContext->m_fsContext, buffer, texture->getWidth() * 2);

            //     // double size of texture
            //     texture->resize(texture->getWidth() * 2, texture->getHeight() * 2);

            //     solved = true; // error solved
            // }
            
            break;
        }

        default:
            logMsg("[FontContext] FontError : undefined error\n");
            break;
    }

    return solved;
}

void FontContext::initFontContext(const std::string& _fontFile, int _atlasSize) {
    GLFONSparams params;

    params.errorCallback = errorCallback;
    params.createAtlas = createAtlas;
    params.createTexTransforms = createTexTransforms;
    params.updateAtlas = updateAtlas;
    params.updateTransforms = updateTransforms;

    m_fsContext = glfonsCreate(_atlasSize, _atlasSize, FONS_ZERO_TOPLEFT, params, (void*) this);

    unsigned int dataSize;
    unsigned char* data = bytesFromResource(_fontFile.c_str(), &dataSize);
    m_font = fonsAddFont(m_fsContext, "droid-serif", data, dataSize);

    if (m_font == FONS_INVALID) {
        logMsg("[FontContext] Error loading font file %s\n", _fontFile.c_str());
    }
}
