#include "shaping.h"

#include "platform.h"
#include <locale>
#include <codecvt>

namespace Tangram {

void Shaping::icuBidiError(UBiDi* _bidi, UErrorCode _error) {
    LOGW("UBidi error %s", u_errorName(_error));
    ubidi_close(_bidi);
}

bool Shaping::bidiDetection(const std::string& _text, hb_direction_t& _direction) {
    std::wstring_convert<std::codecvt_utf8<char16_t>, char16_t> conv;
    std::u16string u16 = conv.from_bytes(_text.c_str());

    // Ubidi detection
    UErrorCode error = U_ZERO_ERROR;
    UBiDiLevel dirn = 0;
    UBiDi* bidi = ubidi_openSized(u16.length(), 0, &error);

    if (error != U_ZERO_ERROR) {
        Shaping::icuBidiError(bidi, error);
        return false;
    }

    ubidi_setPara(bidi, reinterpret_cast<const UChar*>(u16.c_str()), u16.length(), dirn, NULL, &error);

    if (error != U_ZERO_ERROR) {
        Shaping::icuBidiError(bidi, error);
        return false;
    }

    UBiDiDirection direction = ubidi_getDirection(bidi);
    if (direction == UBIDI_MIXED) {
        size_t count = ubidi_countRuns(bidi, &error);

        if (error != U_ZERO_ERROR) {
            Shaping::icuBidiError(bidi, error);
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

hb_direction_t Shaping::icuDirectionToHB(UBiDiDirection _direction) {
    return (_direction == UBIDI_RTL) ? HB_DIRECTION_RTL : HB_DIRECTION_LTR;
}

bool Shaping::scriptDetection(const std::string& _text, hb_script_t& _script) {
    icu_52::UnicodeString text(_text.c_str());
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

