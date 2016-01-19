#pragma once

#include "rapidjson/document.h"

namespace Tangram {

    using JsonDocument = rapidjson::Document;
    using JsonValue = rapidjson::Value;

    JsonDocument JsonParseBytes(const char* _bytes, size_t _length, const char** _error, size_t* _errorOffset);

}
