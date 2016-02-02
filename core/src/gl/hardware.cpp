#include "hardware.h"

#include <cstring>
#include <sstream>
#include <algorithm>
#include <iterator>
#include "platform.h"
#include "gl.h"

namespace Tangram {
namespace Hardware {

bool supportsMapBuffer = false;
bool supportsVAOs = false;
int maxTextureSize = 0;
static char* s_glExtensions;

bool isAvailable(std::string _extension) {
    return bool(s_glExtensions)
      ? strstr(s_glExtensions, _extension.c_str()) != nullptr
      : false;
}

void printAvailableExtensions() {
    if (s_glExtensions == NULL) {
        LOGW("Extensions string is NULL");
        return;
    }

    std::string exts(s_glExtensions);
    std::stringstream ss(exts);

    ss >> std::noskipws;
    std::string extension;
    LOGD("GL Extensions available: ");
    while (std::getline(ss, extension, ' ')) {
        LOGD("\t - %s", extension.c_str());
    }
}

void loadExtensions() {
    s_glExtensions = (char*) glGetString(GL_EXTENSIONS);

    if (s_glExtensions == NULL) {
        LOGE("glGetString( GL_EXTENSIONS ) returned NULL");
        return;
    }

    supportsMapBuffer = DESKTOP_GL || isAvailable("mapbuffer");
    supportsVAOs = isAvailable("vertex_array_object");

    LOG("Driver supports map buffer: %d", supportsMapBuffer);
    LOG("Driver supports vaos: %d", supportsVAOs);

    // find extension symbols if needed
    initGLExtensions();
}

void loadCapabilities() {
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);

    LOG("Hardware max texture size %d", maxTextureSize);
}

}
}
