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
static char* s_glExtensions;

bool isAvailable(std::string _extension) {
    return strstr(s_glExtensions, _extension.c_str());
}

void load(bool _log) {
    s_glExtensions = (char*) glGetString(GL_EXTENSIONS);

    supportsMapBuffer = isAvailable("mapbuffer");

    LOG("Driver support map buffer %d", supportsMapBuffer);

    if (!_log) {
        return;
    }

    std::string exts(s_glExtensions);
    std::istringstream iss(exts);
    std::vector<std::string> extensions;
    std::copy(std::istream_iterator<std::string>(iss), std::istream_iterator<std::string>(), back_inserter(extensions));

    LOG("GL Extensions available: ");
    for (auto ext : extensions) {
        LOG("\t %s", ext.c_str());
    }
}

}
}
