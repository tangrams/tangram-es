#pragma once

#define RAPIDJSON_HAS_STDSTRING 1
#include "util/jsonTypes.h"
#include "rapidjson/document.h"

namespace Tangram {

JsonDocument JsonParseBytes(const char* _bytes, size_t _length, const char** _error, size_t* _errorOffset);

}
