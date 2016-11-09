#pragma once

#include <vector>
#include <string.h>

namespace Tangram {
namespace zlib {

int inflate(const char* _data, size_t _size, std::vector<char>& dst);

}
}
