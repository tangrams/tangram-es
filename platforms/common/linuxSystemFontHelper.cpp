#include "linuxSystemFontHelper.h"
#include "log.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>

namespace Tangram {

std::vector<std::string> systemFallbackFonts(FcConfig* fcConfig) {

    std::vector<std::string> fallbackFonts;

    FcPattern *pat = FcPatternBuild (0,
                          FC_LANG, FcTypeString, "en",
                          FC_WIDTH, FcTypeInteger, FC_WIDTH_NORMAL,
                          FC_WEIGHT, FcTypeInteger, FC_WEIGHT_NORMAL,
                          FC_SLANT, FcTypeInteger, FC_SLANT_ROMAN,
                          FC_SCALABLE, FcTypeBool, FcTrue,
                          (char *) 0);

    FcConfigSubstitute(fcConfig, pat, FcMatchPattern);
    FcDefaultSubstitute(pat);

    FcResult result;
    FcFontSet *fs = FcFontSort(fcConfig, pat, FcTrue, nullptr, &result);
    FcPatternDestroy(pat);

    if (!fs || (result != FcResultMatch)) {
        return fallbackFonts;
    }

    FcLangSet *fls = FcLangSetCreate();
    FcLangSet *empty = FcLangSetCreate();

    for (int i = 0; i < fs->nfont; i++) {
        FcPattern *font = fs->fonts[i];

        int w;
        if (FcPatternGetInteger(font, FC_WEIGHT, 0, &w) != FcResultMatch ||
            (w != FC_WEIGHT_REGULAR)) { continue; }

        if (FcPatternGetInteger(font, FC_SLANT, 0, &w) != FcResultMatch ||
            (w != FC_SLANT_ROMAN)) { continue; }

#ifdef FC_COLOR
        if (FcPatternGetBool(font, FC_COLOR, 0, &w) != FcResultMatch ||
            (w != FcFalse)) { continue; }
#endif

        if (FcPatternGetBool(font, FC_SCALABLE, 0, &w) != FcResultMatch ||
            (w != FcTrue)) { continue; }

        FcChar8 *file = nullptr;
        if (FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch) {
            FcLangSet *ls = nullptr;
            if (FcPatternGetLangSet(font, FC_LANG, 0, &ls) != FcResultMatch) {
                continue;
            }

            FcLangSet *r = FcLangSetSubtract(ls, fls);
            if (!FcLangSetEqual(r, empty)) {
                FcLangSet *u = FcLangSetUnion(r, fls);
                FcLangSetDestroy(fls);
                fls = u;

                fallbackFonts.emplace_back(reinterpret_cast<char*>(file));
            }
            FcLangSetDestroy(r);
        }
    }

    FcFontSetDestroy(fs);
    if (fls) FcLangSetDestroy(fls);
    if (empty) FcLangSetDestroy(empty);

    return fallbackFonts;
}

std::string systemFontPath(FcConfig* fcConfig, const std::string& _name,
                           const std::string& _weight, const std::string& _face) {

    if (!fcConfig) { return ""; }

    std::string fontFile = "";
    FcValue fcFamily, fcFace, fcWeight;

    fcFamily.type = fcFace.type = FcType::FcTypeString;
    fcFamily.u.s = reinterpret_cast<const FcChar8*>(_name.c_str());
    fcFace.u.s = reinterpret_cast<const FcChar8*>(_face.c_str());

    fcWeight.type = FcType::FcTypeInteger;
    fcWeight.u.i = atoi(_weight.c_str());
    if (fcWeight.u.i == 0) {
        if (strcasecmp(_weight.c_str(), "normal") == 0) {
            fcWeight.u.i = 400;
        } else if (strcasecmp(_weight.c_str(), "bold") == 0) {
            fcWeight.u.i = 700;
        } else {
            // Could not parse weight value
            fcWeight.u.i = 400;
        }
    }

    // Create a pattern with family, style and weight font properties
    FcPattern* pattern = FcPatternCreate();

    FcPatternAdd(pattern, FC_FAMILY, fcFamily, true);
    FcPatternAdd(pattern, FC_STYLE, fcFace, true);
    FcPatternAdd(pattern, FC_WEIGHT, fcWeight, true);
    //FcPatternPrint(pattern);

    FcConfigSubstitute(fcConfig, pattern, FcMatchPattern);
    FcDefaultSubstitute(pattern);

    FcResult res;
    FcPattern* font = FcFontMatch(fcConfig, pattern, &res);
    if (font) {
        FcChar8* file = nullptr;
        FcChar8* fontFamily = nullptr;
        if (FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch &&
            FcPatternGetString(font, FC_FAMILY, 0, &fontFamily) == FcResultMatch) {
            // We do not want the "best" match, but an "exact" or at least the same "family" match
            // We have fallbacks to cover rest here.
            if (strcmp(reinterpret_cast<const char*>(fontFamily), _name.c_str()) == 0) {
                fontFile = reinterpret_cast<const char*>(file);
            }
        }
        FcPatternDestroy(font);
    }

    FcPatternDestroy(pattern);

    return fontFile;
}

} // namespace Tangram
