#include "gl/hardware.h"

#include "gl/error.h"
#include "gl.h"
#include "log.h"
#include "platform.h"

#include <cstring>
#include <sstream>
#include <algorithm>
#include <iterator>

namespace Tangram {
namespace Hardware {

bool supportsMapBuffer = false;
bool supportsVAOs = false;
bool supportsTextureNPOT = false;
bool supportsGLRGBA8OES = false;

uint32_t maxTextureSize = 0;
uint32_t maxCombinedTextureUnits = 0;
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
    s_glExtensions = (char*) GL::getString(GL_EXTENSIONS);

    if (s_glExtensions == NULL) {
        LOGE("glGetString( GL_EXTENSIONS ) returned NULL");
        return;
    }

    supportsMapBuffer = isAvailable("mapbuffer");
    supportsVAOs = isAvailable("vertex_array_object");
    supportsTextureNPOT = isAvailable("texture_non_power_of_two");
    supportsGLRGBA8OES = isAvailable("rgb8_rgba8");

    LOG("Driver supports map buffer: %d", supportsMapBuffer);
    LOG("Driver supports vaos: %d", supportsVAOs);
    LOG("Driver supports rgb8_rgba8: %d", supportsGLRGBA8OES);
    LOG("Driver supports NPOT texture: %d", supportsTextureNPOT);

    // find extension symbols if needed
    initGLExtensions();
}

void loadCapabilities() {
    int val;
    GL::getIntegerv(GL_MAX_TEXTURE_SIZE, &val);
    maxTextureSize = val;

    GL::getIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &val);
    maxCombinedTextureUnits = val;

    LOG("Hardware max texture size %d", maxTextureSize);
    LOG("Hardware max combined texture units %d", maxCombinedTextureUnits);
}

}
}
