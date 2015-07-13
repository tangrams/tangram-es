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

class TextBatch;

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

    /* sets the screen size, this size is used when transforming text ids in the text buffers */
    void setScreenSize(int _width, int _height);

    /* fills the orthographic projection matrix related to the current screen size */
    void getProjection(float* _projectionMatrix) const;

    void clearState();

    /* lock thread access to this font context */
    void lock();

    /* unlock thread access to this font context */
    void unlock();

    const std::unique_ptr<Texture>& getAtlas() const;

    FONScontext* getFontContext() const { return m_fsContext; }

    struct ScopedBinding {
        ScopedBinding(FontContext& _ctx, fsuint _buffer) : m_ctx(_ctx) {
            m_ctx.m_contextMutex.lock();
            glfonsBindBuffer(m_ctx.m_fsContext, _buffer);
        }

        ~ScopedBinding() {
            glfonsBindBuffer(m_ctx.m_fsContext, 0);
            m_ctx.m_contextMutex.unlock();
        }

        FONScontext *get() { return m_ctx.m_fsContext; }

        FontContext& m_ctx;
    };

    ScopedBinding bind(fsuint _buffer) {
        return ScopedBinding(*this, _buffer);
    }

private:

    void initFontContext(int _atlasSize);

    std::map<std::string, int> m_fonts;
    std::unique_ptr<Texture> m_atlas;
    mutable std::mutex m_contextMutex;
    FONScontext* m_fsContext;

};
