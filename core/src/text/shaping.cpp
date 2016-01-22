#include "shaping.h"

#include "platform.h"

#include "hb-ft.h"

#include "unicode/unistr.h"
#include "unicode/ubidi.h"
#include "unicode/uscript.h"
#include "unicode/schriter.h"

namespace Tangram {

void icuBidiError(UBiDi* _bidi, UErrorCode _error) {
    LOGW("UBidi error %s", u_errorName(_error));
    ubidi_close(_bidi);
}

hb_direction_t icuDirectionToHB(UBiDiDirection _direction) {
    return (_direction == UBIDI_RTL) ? HB_DIRECTION_RTL : HB_DIRECTION_LTR;
}

bool Shaping::bidiDetection(const std::string& _text, hb_direction_t& _direction) {
    auto text = UnicodeString::fromUTF8(_text);

    // Ubidi detection
    UErrorCode error = U_ZERO_ERROR;
    UBiDiLevel dirn = 0;
    UBiDi* bidi = ubidi_openSized(text.length(), 0, &error);

    if (error != U_ZERO_ERROR) {
        icuBidiError(bidi, error);
        return false;
    }

    ubidi_setPara(bidi, text.getBuffer(), text.length(), dirn, NULL, &error);

    if (error != U_ZERO_ERROR) {
        icuBidiError(bidi, error);
        return false;
    }

    UBiDiDirection direction = ubidi_getDirection(bidi);
    if (direction == UBIDI_MIXED) {
        size_t count = ubidi_countRuns(bidi, &error);

        if (error != U_ZERO_ERROR) {
            icuBidiError(bidi, error);
            return false;
        }

        if (count > 1) {
            // Multiple bidi detected, no fallback for now
            LOGW("Can't draw string %s -- multiple bidi detected", _text.c_str());
            ubidi_close(bidi);

            // for (size_t i = 0; i < count; i++) {
            //     int start, length;
            //     direction = ubidi_getVisualRun_52(bidi, i, &start, &length);
            // }
            return false;
        }
    }

    _direction = icuDirectionToHB(direction);

    ubidi_close(bidi);

    return true;
}

bool Shaping::scriptDetection(const std::string& _text, hb_script_t& _script) {
    auto text = UnicodeString::fromUTF8(_text);

    hb_unicode_funcs_t* unicodeFuncs =  hb_unicode_funcs_get_default();
    bool invalid = true;

    _script = HB_SCRIPT_INVALID;

    for (int i = 0; i < text.length(); ++i) {
        hb_codepoint_t codepoint = (hb_codepoint_t)text.char32At(i);
        _script = hb_unicode_script(unicodeFuncs, codepoint);

        if (_script != HB_SCRIPT_INVALID) {
            invalid = false;
            break;
        }
    }

    if (invalid) {
        LOGW("Could not determine script from string %s", _text.c_str());
        return false;
    }

    return true;
}

}
