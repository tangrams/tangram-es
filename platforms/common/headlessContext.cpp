#include "headlessContext.h"

#include "log.h"

#define IMAGE_DEPTH 4

namespace Tangram {

bool HeadlessContext::init() {

   m_ctx = OSMesaCreateContextExt(OSMESA_RGBA, 16, 8, 0, NULL );
   if (!m_ctx) {
      LOGE("OSMesaCreateContext failed!");
      return false;
   }

   return true;
}

bool HeadlessContext::resize(uint32_t w, uint32_t h) {

    if (m_width == int(w) && m_height == int(h)) {
        return true;
    }

    if (m_buffer) {
        free(m_buffer);
        m_buffer = nullptr;
    }

    m_width = w;
    m_height = h;

    m_buffer = static_cast<GLubyte*>(malloc(m_width * m_height * IMAGE_DEPTH * sizeof(GLubyte)));
    if (!m_buffer) {
        LOGE("Alloc image buffer failed!");
        return false;
    }

    return true;
}

bool HeadlessContext::makeCurrent() {
    if (!m_buffer) {
        return false;
    }
    if (!OSMesaMakeCurrent(m_ctx, m_buffer, GL_UNSIGNED_BYTE, m_width, m_height)) {
        LOGE("OSMesaMakeCurrent failed!");
        return false;
    }
    OSMesaPixelStore(OSMESA_Y_UP, 0);

    return true;
}

void HeadlessContext::destroy() {
    if (m_ctx) {
        OSMesaDestroyContext(m_ctx);
        m_ctx = nullptr;
    }
}

bool HeadlessContext::writeImage(const char *filename) {
    if (!m_buffer) { return false; }

    FILE *f = fopen(filename, "w");
    if (!f) { return false; }

    const GLubyte *ptr = m_buffer;
    fprintf(f,"P6\n");
    fprintf(f,"# ppm-file created by Tangram\n");
    fprintf(f,"%i %i\n", m_width, m_height);
    fprintf(f,"255\n");
    fclose(f);

    f = fopen(filename, "ab");

    for (int y = 0; y < m_height; y++) {
        for (int x = 0; x < m_width; x++) {
            int i = (y * m_width + x) * IMAGE_DEPTH;
            fputc(ptr[i], f);
            fputc(ptr[i+1], f);
            fputc(ptr[i+2], f);
        }
    }
    fclose(f);
    return true;
}

HeadlessContext::HeadlessContext() {}

HeadlessContext::~HeadlessContext() {
    if (m_buffer) {

        writeImage("a.ppm");

        free(m_buffer);
        m_buffer = nullptr;
    }
    destroy();
}

}
