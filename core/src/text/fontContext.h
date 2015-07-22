#pragma once

#include "gl.h"
#include "glfontstash.h"
#include "gl/texture.h"
#include "platform.h"
#include "textBuffer.h"
#include "stl_util.hpp"
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <map>

namespace Tangram {

class TextBuffer;

class FontContext {

public:

    FontContext();
    FontContext(int _atlasSize);
    ~FontContext();

    /* adds a font from a .ttf font file with a specific name */
    bool addFont(const std::string& _fontFile, std::string _name);

    /* sets the current font for a size in pixels */
    void setFont(const std::string& _name, int size);

    /* sets the blur spread when using signed distance field rendering */
    void setSignedDistanceField(float _blurSpread);

    void clearState();
    
    /* lock thread access to this font context */
    void lock();

    /* unlock thread access to this font context */
    void unlock();

    const std::unique_ptr<Texture>& getAtlas() const;

    FONScontext* getFontContext() const { return m_fsContext; }

private:

    void initFontContext(int _atlasSize);

    std::map<std::string, int> m_fonts;
    std::unique_ptr<Texture> m_atlas;
    std::unique_ptr<std::mutex> m_contextMutex;
    FONScontext* m_fsContext;

};

}
