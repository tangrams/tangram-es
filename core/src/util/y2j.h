#pragma once

#include "util/json.h"

namespace Tangram {

JsonDocument yamlParseBytes(const char* bytes, size_t length, const char** errorMessage, size_t* errorLine);

} // namespace Tangram
