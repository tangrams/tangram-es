#pragma once

#include <string>

#include "hb.h"
#include "hb-ft.h"

#include "unicode/unistr.h"
#include "unicode/ubidi.h"
#include "unicode/uscript.h"
#include "unicode/schriter.h"

namespace Tangram {

class Shaping {

public:
    static bool bidiDetection(const std::string& _text, hb_direction_t& _direction);
    static bool scriptDetection(const std::string& _text, hb_script_t& _script);

private:
    static void icuBidiError(UBiDi* _bidi, UErrorCode _error);
    static hb_direction_t icuDirectionToHB(UBiDiDirection _direction);

};

}
