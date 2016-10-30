#pragma once

#include "util/json.h"

namespace y2j {

JsonDocument yamlParseBytes(const char* bytes, size_t length, const char** errorMessage, size_t* errorLine);

} // namespace y2j
