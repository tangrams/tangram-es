#pragma once

#include <fontconfig/fontconfig.h>

#include "platform.h"

namespace Tangram {

std::vector<std::string> systemFallbackFonts(FcConfig* fcConfig);

std::string systemFontPath(FcConfig* fcConfig,
        const std::string& _name, const std::string& _weight, const std::string& _face);

} // namespace Tangram
