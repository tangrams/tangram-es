#pragma once

#include "gl.h"
#include "fontstash.h"
#include "gl/texture.h"
#include "platform.h"
#include "stl_util.hpp"
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <map>

namespace Tangram {

typedef unsigned int FontID;

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

    FontID getFontID(const std::string& _name);

    /* sets the blur spread when using signed distance field rendering */
    void setSignedDistanceField(float _blurSpread);

    void clearState();

    /* lock thread access to this font context */
    void lock();

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

    void initFontContext(int _atlasSize);

    std::map<std::string, int> m_fonts;
    std::unique_ptr<Texture> m_atlas;
    std::mutex m_contextMutex;
    std::mutex m_atlasMutex;

    FONScontext* m_fsContext;
    std::vector<FONSquad> m_quadBuffer;

};

}
