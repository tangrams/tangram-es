#pragma once

#include "platform_gl.h"

#include "GL/osmesa.h"

namespace Tangram {

class HeadlessContext {
public:

    HeadlessContext();
    ~HeadlessContext();

    bool init();
    bool resize(uint32_t w, uint32_t h);
    bool makeCurrent();
    void destroy();
    bool writeImage(const char *filename);

private:
    GLubyte* m_buffer = nullptr;
    OSMesaContext m_ctx;

    int m_width = -1;
    int m_height = -1;

};

}
