#pragma once

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

    bool addFont(std::string _fontFile, std::string _name);
    void setFont(std::string _name, int size);
    void setSignedDistanceField(float _blurSpread);
    void setScreenSize(int _width, int _height);
    void getProjection(float* _projectionMatrix) const;

    std::shared_ptr<TextBuffer> genTextBuffer();

    void useBuffer(const std::shared_ptr<TextBuffer>& _textBuffer);
    std::shared_ptr<TextBuffer> getCurrentBuffer(); 

    void clearState();

    void lock();
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
