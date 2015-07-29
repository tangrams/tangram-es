#pragma once

#include "gl.h"
#include "glfontstash.h"
#include "gl/texture.h"
#include "platform.h"
#include "textBuffer.h"
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <map>

namespace Tangram {

class TextBuffer;

class FontContext {

public:

    static std::shared_ptr<FontContext> GetInstance() {
        static std::shared_ptr<FontContext> instance(new FontContext());
        return instance;
    }

    FontContext();
    FontContext(int _atlasSize);
    ~FontContext();

    /* adds a font from a .ttf font file with a specific name */
    bool addFont(const std::string& _fontFile, std::string _name);

    /* sets the current font for a size in pixels */
    void setFont(const std::string& _name, int size);

    fsuint getFontID(const std::string& _name);

    /* sets the blur spread when using signed distance field rendering */
    void setSignedDistanceField(float _blurSpread);

    void clearState();
    
    /* lock thread access to this font context */
    void lock();

    /* unlock thread access to this font context */
    void unlock();

    void bindAtlas(GLuint _textureUnit);

    FONScontext* getFontContext() const { return m_fsContext; }

private:

    static void updateAtlas(void* _userPtr, unsigned int _xoff, unsigned int _yoff,
                            unsigned int _width, unsigned int _height, const unsigned int* _pixels);

    void initFontContext(int _atlasSize);

    std::map<std::string, int> m_fonts;
    std::unique_ptr<Texture> m_atlas;
    std::mutex m_contextMutex;
    std::mutex m_atlasMutex;

    FONScontext* m_fsContext;

};

}
