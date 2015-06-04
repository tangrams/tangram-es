#pragma once

#include "gl.h"
#include "glfontstash.h"
#include "texture.h"
#include "platform.h"
#include "textBuffer.h"
#include "stl_util.hpp"
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <map>

class TextBuffer;

class FontContext {

public:

    FontContext();
    FontContext(int _atlasSize);
    ~FontContext();

    /* adds a font from a .ttf font file with a specific name */
    bool addFont(std::string _fontFile, std::string _name);

    /* sets the current font for a size in pixels */
    void setFont(std::string _name, int size);

    /* sets the blur spread when using signed distance field rendering */
    void setSignedDistanceField(float _blurSpread);

    /* sets the screen size, this size is used when transforming text ids in the text buffers */
    void setScreenSize(int _width, int _height);

    /* fills the orthographic projection matrix related to the current screen size */
    void getProjection(float* _projectionMatrix) const;

    /* asks to generate an uninitialized text buffer */
    std::shared_ptr<TextBuffer> genTextBuffer();

    /* use a buffer : all callbacks related to text buffer would be done on this buffer */
    void useBuffer(const std::shared_ptr<TextBuffer>& _textBuffer);

    /* gets the currently used buffer by the font context */
    std::shared_ptr<TextBuffer> getCurrentBuffer(); 

    void clearState();

    /* lock thread access to this font context */
    void lock();
    
    /* unlock thread access to this font context */
    void unlock();

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

    void initFontContext(int _atlasSize);

    std::map<std::string, int> m_fonts;
    std::weak_ptr<TextBuffer> m_currentBuffer;
    std::unique_ptr<Texture> m_atlas;
    std::unique_ptr<std::mutex> m_contextMutex;
    FONScontext* m_fsContext;

    int m_font;
    
};
