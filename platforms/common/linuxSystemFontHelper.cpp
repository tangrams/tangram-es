#include "linuxSystemFontHelper.h"

#include <algorithm>
#include <cstring>

namespace Tangram {

std::vector<std::string> systemFallbackFonts(FcConfig* fcConfig) {

    std::string style = "Regular";
    std::vector<std::string> fallbackFonts;

    /*
     * Read system fontconfig to get list of fallback font for each supported language
     */
    FcStrSet* fcLangs = FcGetLangs();
    FcStrList* fcLangList = FcStrListCreate(fcLangs);
    FcChar8* fcLang;
    while ((fcLang = FcStrListNext(fcLangList))) {
        FcValue fcStyleValue, fcLangValue;

        fcStyleValue.type = fcLangValue.type = FcType::FcTypeString;
        fcStyleValue.u.s = reinterpret_cast<const FcChar8*>(style.c_str());
        fcLangValue.u.s = fcLang;

        // create a pattern with style and family font properties
        FcPattern* pat = FcPatternCreate();

        FcPatternAdd(pat, FC_STYLE, fcStyleValue, true);
        FcPatternAdd(pat, FC_LANG, fcLangValue, true);
        //FcPatternPrint(pat);

        FcConfigSubstitute(fcConfig, pat, FcMatchPattern);
        FcDefaultSubstitute(pat);

        FcResult res;
        FcPattern* font = FcFontMatch(fcConfig, pat, &res);
        if (font) {
            FcChar8* file = nullptr;
            if (FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch) {
                // Make sure this font file is not previously added.
                if (std::find(fallbackFonts.begin(), fallbackFonts.end(),
                              reinterpret_cast<char*>(file)) == fallbackFonts.end()) {
                    fallbackFonts.emplace_back(reinterpret_cast<char*>(file));
                }
            }
            FcPatternDestroy(font);
        }
        FcPatternDestroy(pat);
    }
    FcStrListDone(fcLangList);

    return fallbackFonts;
}

std::string systemFontPath(FcConfig* fcConfig,
        const std::string& _name, const std::string& _weight, const std::string& _face) {

    if (!fcConfig) { return ""; }

    std::string fontFile = "";
    FcValue fcFamily, fcFace, fcWeight;

    fcFamily.type = fcFace.type = fcWeight.type = FcType::FcTypeString;
    fcFamily.u.s = reinterpret_cast<const FcChar8*>(_name.c_str());
    fcWeight.u.s = reinterpret_cast<const FcChar8*>(_weight.c_str());
    fcFace.u.s = reinterpret_cast<const FcChar8*>(_face.c_str());

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
