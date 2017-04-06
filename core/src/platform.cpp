#include "platform.h"
#include "log.h"

#include <fstream>
#include <string>

namespace Tangram {

Platform::Platform() : m_continuousRendering(false) {}

Platform::~Platform() {}

void Platform::setContinuousRendering(bool _isContinuous) {
    m_continuousRendering = _isContinuous;
}

bool Platform::isContinuousRendering() const {
    return m_continuousRendering;
}

bool Platform::bytesFromFileSystem(const char* _path, std::function<char*(size_t)> _allocator) const {
    std::ifstream resource(_path, std::ifstream::ate | std::ifstream::binary);

    if(!resource.is_open()) {
        LOGW("Failed to read file at path: %s", _path);
        return false;
    }

    size_t size = resource.tellg();
    char* cdata = _allocator(size);

    resource.seekg(std::ifstream::beg);
    resource.read(cdata, size);
    resource.close();

    return true;
}

std::vector<char> Platform::systemFont(const std::string& _name, const std::string& _weight, const std::string& _face) const {
    // No-op by default
    return {};
}

std::vector<FontSourceHandle> Platform::systemFontFallbacksHandle() const {
    // No-op by default
    return {};
}

} // namespace Tangram
