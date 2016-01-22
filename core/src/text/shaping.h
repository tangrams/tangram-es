#pragma once

#include <string>

#include "hb.h"

namespace Tangram {

class Shaping {

    Shaping() = delete;

public:
    static bool bidiDetection(const std::string& _text, hb_direction_t& _direction);
    static bool scriptDetection(const std::string& _text, hb_script_t& _script);

};

}
