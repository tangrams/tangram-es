#include "linuxSystemFontHelper.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>

#include <chrono>
#include <log.h>

namespace Tangram {

std::chrono::time_point<std::chrono::system_clock> t_start, t_last;
#define LOGTimeInit() do { t_last = t_start = std::chrono::system_clock::now(); } while(0)
#define LOGTime(msg) do { \
        std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now(); \
        std::chrono::duration<double> t1 = now - t_start;               \
        std::chrono::duration<double> t2 = now - t_last;                \
        t_last = now;                                                   \
        LOGW(">>>> " msg " %.6f / %.6f", t1.count(), t2.count()); } while(0)

std::vector<std::string> systemFallbackFonts(FcConfig* fcConfig) {

    LOGTimeInit();

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
    // FcPatternPrint(pat);

    FcResult result;
    FcFontSet *fs = FcFontSort(fcConfig, pat, FcTrue, NULL, &result);
    FcPatternDestroy(pat);

    if (!fs || (result != FcResultMatch)) {
        return fallbackFonts;
    }

    //FcCharSet *fcs = FcCharSetCreate();

    FcLangSet *fls = FcLangSetCreate();
    FcLangSet *empty = FcLangSetCreate();

    int count = 0;
    for (int i = 0; i < fs->nfont; i++) {
        FcPattern *font = fs->fonts[i];

        //FcPatternPrint(font);
        // FcChar8 *s = FcNameUnparse(font);
        // printf("Font: %s\n", s);
        // free(s);

        int w;
        if (FcPatternGetInteger(font, FC_WEIGHT, 0, &w) != FcResultMatch || w != FC_WEIGHT_REGULAR) {
            continue;
        }
        if (FcPatternGetInteger(font, FC_SLANT, 0, &w) != FcResultMatch || w != FC_SLANT_ROMAN) {
            continue;
        }
        if (FcPatternGetBool(font, FC_COLOR, 0, &w) != FcResultMatch || w != FcFalse) {
            continue;
        }
        if (FcPatternGetBool(font, FC_SCALABLE, 0, &w) != FcResultMatch || w != FcTrue) {
            continue;
        }
        // if (FcPatternGetBool(font, FC_VARIABLE, 0, &w) != FcResultMatch || w != FcTrue) {
        //     continue;
        // }

        // FcCharSet *cs = NULL;
        // if (FcPatternGetCharSet(font, FC_CHARSET, 0, &cs) != FcResultMatch) {
        //     LOGW("no charset!");
        //     continue;
        // }
        // if (FcCharSetIsSubset(cs, fcs)) {
        //     LOGW("charset included!");
        //     continue;
        // }
        // LOGW("Add chars: %d", FcCharSetSubtractCount(cs, fcs));

        // FcCharSet *u = FcCharSetUnion(cs, fcs);
        // FcCharSetDestroy(fcs);
        // fcs = u;
        FcChar8 *file;

        if (FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch) {
            FcLangSet *ls = NULL;
            if (FcPatternGetLangSet(font, FC_LANG, 0, &ls) != FcResultMatch) {
                continue;
            }

            FcLangSet *r = FcLangSetSubtract(ls, fls);
            if (FcLangSetEqual(r, empty)) {
                //printf("langset included! %s\n", reinterpret_cast<char*>(file));
                continue;
            }

            // FcStrSet *langs = FcLangSetGetLangs(r);
            // FcStrList *ll = FcStrListCreate(langs);
            // FcChar8 *l;
            // FcStrListFirst(ll);
            // while ((l = FcStrListNext(ll)) != NULL) {
            //     printf(">%s\n", l);
            // }
            // FcStrListDone(ll);
            // FcStrSetDestroy(langs);

            FcLangSet *u = FcLangSetUnion(ls, fls);
            FcLangSetDestroy(fls);
            fls = u;

            if (std::find(fallbackFonts.begin(), fallbackFonts.end(),
                          reinterpret_cast<char*>(file)) == fallbackFonts.end()) {
                fallbackFonts.emplace_back(reinterpret_cast<char*>(file));
                LOGW("Add: %s", reinterpret_cast<char*>(file));
                count++;
            }
        }
    }

    LOGW("Count %d / %d", count, fs->nfont);

    FcFontSetDestroy(fs);
    // if (fcs) FcCharSetDestroy(fcs);

    if (fls) FcLangSetDestroy(fls);
    if (empty) FcLangSetDestroy(empty);

    LOGTime("check");

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
