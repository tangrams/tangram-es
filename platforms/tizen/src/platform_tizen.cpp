#include "platform_tizen.h"
#include "gl/hardware.h"

#include <algorithm>
#include <stdio.h>
#include <stdarg.h>
#include <libgen.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/syscall.h>

#include <dlog.h>
#include <fontconfig/fontconfig.h>

#ifdef  LOG_TAG
#undef  LOG_TAG
#endif
#define LOG_TAG "Tangram"

namespace Tangram {

static std::vector<std::string> s_fallbackFonts;
static FcConfig* s_fcConfig = nullptr;

void logMsg(const char* fmt, ...) {
    va_list vl;
    va_start(vl, fmt);
    dlog_vprint(DLOG_WARN, LOG_TAG, fmt, vl);
    va_end(vl);
}

TizenPlatform::TizenPlatform() :
    m_urlClient(UrlClient::Options{}) {}

TizenPlatform::TizenPlatform(UrlClient::Options urlClientOptions) :
    m_urlClient(urlClientOptions) {}

TizenPlatform::~TizenPlatform() {}

void TizenPlatform::setRenderCallbackFunction(std::function<void()> callback) {
    m_renderCallbackFunction = callback;
}

void TizenPlatform::requestRender() const {
    m_update = true;
    if (m_renderCallbackFunction) {
        m_renderCallbackFunction();
    }
}

bool TizenPlatform::startUrlRequest(const std::string& _url, UrlCallback _callback) {

    return m_urlClient.addRequest(_url, _callback);
}

void TizenPlatform::cancelUrlRequest(const std::string& _url) {

    m_urlClient.cancelRequest(_url);
}

void TizenPlatform::initPlatformFontSetup() const {

    static bool s_platformFontsInit = false;
    if (s_platformFontsInit) { return; }
    s_platformFontsInit = true;

    s_fcConfig = FcInitLoadConfigAndFonts();

    return;

    std::string style = "Regular";

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

        FcConfigSubstitute(s_fcConfig, pat, FcMatchPattern);
        FcDefaultSubstitute(pat);

        FcResult res;
        FcPattern* font = FcFontMatch(s_fcConfig, pat, &res);

        if (font) {
            FcChar8* file = nullptr;
            bool hasFile = FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch;
            logMsg("TANGRAM: Fallback: %s - %s\n", fcLang, file);

            if (hasFile) {
                // Make sure this font file is not previously added.
                if (std::find(s_fallbackFonts.begin(), s_fallbackFonts.end(),
                              reinterpret_cast<char*>(file)) == s_fallbackFonts.end()) {
                    s_fallbackFonts.emplace_back(reinterpret_cast<char*>(file));
                }
            }
            FcPatternDestroy(font);
        }
        FcPatternDestroy(pat);
    }
    FcStrListDone(fcLangList);
}

std::vector<FontSourceHandle> TizenPlatform::systemFontFallbacksHandle() const {

    // initPlatformFontSetup();

    std::vector<FontSourceHandle> handles;

    handles.emplace_back("/usr/share/fonts/SamsungOneUI-RegularCondensed.ttf");
    handles.emplace_back("/usr/share/fallback_fonts/BreezeSansArmenian-Regular.ttf");
    handles.emplace_back("/usr/share/fallback_fonts/BreezeSansBengali-Regular.ttf");
    handles.emplace_back("/usr/share/fallback_fonts/BreezeSansGeorgian-Regular.ttf");
    handles.emplace_back("/usr/share/fallback_fonts/BreezeSansGujarathi-Regular.ttf");
    handles.emplace_back("/usr/share/fallback_fonts/BreezeSansHindi-Regular.ttf");
    handles.emplace_back("/usr/share/fallback_fonts/BreezeSansJapanese-Regular.ttf");
    handles.emplace_back("/usr/share/fallback_fonts/BreezeSansKannada-Regular.ttf");
    handles.emplace_back("/usr/share/fallback_fonts/BreezeSansKhmer-Regular.ttf");
    handles.emplace_back("/usr/share/fallback_fonts/BreezeSansMalayalam-Regular.ttf");
    handles.emplace_back("/usr/share/fallback_fonts/BreezeSansMyanmar-Regular.ttf");
    handles.emplace_back("/usr/share/fallback_fonts/BreezeSansPunjabi-Regular.ttf");
    handles.emplace_back("/usr/share/fallback_fonts/BreezeSansSinhala-Regular.ttf");
    handles.emplace_back("/usr/share/fallback_fonts/BreezeSansTamil-Regular.ttf");
    handles.emplace_back("/usr/share/fallback_fonts/BreezeSansTelugu-Regular.ttf");
    handles.emplace_back("/usr/share/fallback_fonts/BreezeSansThai-Regular.ttf");
    handles.emplace_back("/usr/share/fallback_fonts/SamsungOneUIArabic-Regular.ttf");
    handles.emplace_back("/usr/share/fallback_fonts/SamsungOneUIEthiopic-Regular.ttf");
    handles.emplace_back("/usr/share/fallback_fonts/SamsungOneUIHebrew-Regular.ttf");
    handles.emplace_back("/usr/share/fallback_fonts/SamsungOneUIKorean-RegularCondensed.ttf");
    handles.emplace_back("/usr/share/fallback_fonts/SamsungOneUILao-Regular.ttf");

    // for (auto& path : s_fallbackFonts) {
    //     handles.emplace_back(path);
    // }

    return handles;
}

std::string TizenPlatform::fontPath(const std::string& _name, const std::string& _weight,
                                    const std::string& _face) const {

    initPlatformFontSetup();

    if (!s_fcConfig) {
        return "";
    }

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

    FcConfigSubstitute(s_fcConfig, pattern, FcMatchPattern);
    FcDefaultSubstitute(pattern);

    FcResult res;
    FcPattern* font = FcFontMatch(s_fcConfig, pattern, &res);
    if (font) {

        FcChar8* file = nullptr;
        FcChar8* fontFamily = nullptr;

        bool hasFile = FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch;

        logMsg("TANGRAM: Font: %s\n", file);

        if (hasFile &&
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

std::vector<char> TizenPlatform::systemFont(const std::string& _name, const std::string& _weight,
                                            const std::string& _face) const {
    std::string path = fontPath(_name, _weight, _face);

    if (path.empty()) { return {}; }

    return bytesFromFile(path.c_str());
}


void setCurrentThreadPriority(int priority) {
    int tid = syscall(SYS_gettid);
    setpriority(PRIO_PROCESS, tid, priority);
}

void initGLExtensions() {
}

} // namespace Tangram
