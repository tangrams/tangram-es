#pragma once

#include "glfontstash.h"
#include "texture.h"
#include "platform.h"
#include "textBuffer.h"
#include <memory>
#include <mutex>
#include <string>

class TextBuffer;

class FontContext {

public:
    FontContext(std::string _fontFile);
    FontContext(std::string _fontFile, int _atlasSize);
    ~FontContext();

    void setScreenSize(int _width, int _height);
    void getViewProjection(float* _projectionMatrix) const;

    std::shared_ptr<TextBuffer> genTextBuffer() const;
    std::shared_ptr<TextBuffer> genTextBuffer(int _size) const;

    void bindTextBuffer(const std::shared_ptr<TextBuffer>& _textBuffer);

    const std::unique_ptr<Texture>& getAtlas() const;

    /* Called by fontstash when the texture need to create a new transform textures */
    friend void createTexTransforms(void* _userPtr, unsigned int _width, unsigned int _height);

    /* Called by fontsash when the texture need to be updated */
    friend void updateTransforms(void* _userPtr, unsigned int _xoff, unsigned int _yoff, unsigned int _width,
                            unsigned int _height, const unsigned int* _pixels, void* _ownerPtr);

    /* Called by fontstash when the atlas need to update the atlas texture */
    friend void updateAtlas(void* _userPtr, unsigned int _xoff, unsigned int _yoff,
                            unsigned int _width, unsigned int _height, const unsigned int* _pixels);

    /* Called by fontstash when the atlas need to be created */
    friend void createAtlas(void* _usrPtr, unsigned int _width, unsigned int _height);

    /* Callback on errors */
    friend bool errorCallback(void* _userPtr, fsuint buffer, GLFONSError error);

private:
    void initFontContext(const std::string& _fontFile, int _atlasSize);

    std::weak_ptr<TextBuffer> m_boundBuffer;
    std::unique_ptr<Texture> m_atlas;
    FONScontext* m_fsContext;
    int m_font;
};
