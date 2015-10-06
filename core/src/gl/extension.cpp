#include "extension.h"

#include <cstring>
#include <sstream>
#include <algorithm>
#include <iterator>
#include "platform.h"
#include "gl.h"

namespace Tangram {
namespace GLExtensions {

bool supportsMapBuffer = false;
bool supportsVAOs = false;
static char* s_glExtensions;

bool isAvailable(std::string _extension) {
    return strstr(s_glExtensions, _extension.c_str());
}

void printAvailableExtensions() {
    std::string exts(s_glExtensions);
    std::istringstream iss(exts);
    std::vector<std::string> extensions;
    auto s0 = std::istream_iterator<std::string>(iss);
    auto s1 = std::istream_iterator<std::string>();

    std::copy(s0, s1, back_inserter(extensions));

    LOG("GL Extensions available: ");
    for (auto ext : extensions) {
        LOG("\t %s", ext.c_str());
    }
}

void load() {
    s_glExtensions = (char*) glGetString(GL_EXTENSIONS);

    supportsMapBuffer = isAvailable("mapbuffer");
    supportsVAOs = isAvailable("vertex_array_object");

    LOG("Driver supports map buffer %d", supportsMapBuffer);
    LOG("Driver supports vaos %d", supportsVAOs);

    // find extension symbols if needed
    initGLExtensions();
}

}
}
