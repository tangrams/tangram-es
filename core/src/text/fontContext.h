#pragma once

#include "gl.h"
#include "fontstash.h"
#include "gl/texture.h"
#include "platform.h"
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <map>

namespace Tangram {

typedef int FontID;

class FontContext {

public:


    ~FontContext();
    FontContext();

    /* adds a font from a .ttf font file using "family", "weight" and "style" font properties*/
    FontID addFont(const std::string& _family, const std::string& _weight,
                   const std::string& _style, bool _tryFallback = true);

    /* sets the current font for a size in pixels */
    void setFont(const std::string& _key, int size);

    FontID getFontID(const std::string& _key);

    void clearState();

    /* lock thread access to this font context */
    bool lock();

    /* unlock thread access to this font context */
    void unlock();

    void bindAtlas(GLuint _textureUnit);

    /*
     * Create quads and glyphs for @_text with given options.
     *
     * NB: the returned quads are only valid while <FontContext> is locked!
     */
    std::vector<FONSquad>& rasterize(const std::string& _text, FontID _fontID, float _fontSize, float _sdf);


private:
    static void renderUpdate(void* _userPtr, int* _rect, const unsigned char* _data);
    static int renderCreate(void* _userPtr, int _width, int _height);
    static void pushQuad(void* _userPtr, const FONSquad* _quad);
    static void fontstashError(void* uptr, int error, int val);

    FontContext(int _atlasSize);

    void initFontContext(int _atlasSize);

    std::map<std::string, int> m_fonts;
    std::unique_ptr<Texture> m_atlas;
    std::mutex m_contextMutex;
    std::mutex m_atlasMutex;

    FONScontext* m_fsContext;
    std::vector<FONSquad> m_quadBuffer;

    bool m_atlasFull = false;
};

}
