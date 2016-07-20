#include <stdio.h>
#include <stdarg.h>
#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <memory>
#include <algorithm>
#include <vector>

//#include "urlWorker.h"
#include "platform_tizen.h"
#include "platform_gl.h"

#include <app_common.h>
#include <dlog.h>

#include <libgen.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/syscall.h>

#include <fontconfig.h>


#define NUM_WORKERS 4

static bool s_isContinuousRendering = false;
static std::function<void()> s_renderCallbackFunction = nullptr;

static std::vector<std::string> s_fallbackFonts;
static FcConfig* s_fcConfig = nullptr;

#if USE_ECORE_CON
#include <Eina.h>
#include <Ecore.h>
#include <Ecore_Con.h>

struct UrlTask {
    Ecore_Con_Url *url_con;
    Eina_Binbuf *data;
    UrlCallback cb;
    bool inUse;

    UrlTask (const std::string _url, UrlCallback _cb) : cb(_cb) {
        data = eina_binbuf_new();
        url_con = ecore_con_url_new(_url.c_str());

        ecore_con_url_get(url_con);
    }

    ~UrlTask() {
        eina_binbuf_free(data);
        ecore_con_url_free(url_con);
    }
};

static std::list<std::unique_ptr<UrlTask>> s_urlTaskQueue;

static Eina_Bool data_callback(void *data, int type, void *event) {


    auto *url_data = (Ecore_Con_Event_Url_Data*)event;

    auto task = std::find_if(s_urlTaskQueue.begin(), s_urlTaskQueue.end(),
                           [&](auto& t) { return t->url_con == url_data->url_con; });
    if (task == s_urlTaskQueue.end()) { return true; }

    if (url_data->size > 0) {
        eina_binbuf_append_length((*task)->data, url_data->data, url_data->size);
        //fprintf(stderr, "Appended %d \n", url_data->size);
    }
    return true;
}

static Eina_Bool completion_callback(void *data, int type, void *event) {


    auto* url_complete = (Ecore_Con_Event_Url_Complete*)event;

    auto it = std::find_if(s_urlTaskQueue.begin(), s_urlTaskQueue.end(),
                           [&](auto& t) { return t->url_con == url_complete->url_con; });
    if (it == s_urlTaskQueue.end()) { return true; }
    auto task = std::move(*it);
    s_urlTaskQueue.erase(it);

    if (url_complete->status == 200) {
        const unsigned char *buffer = eina_binbuf_string_get(task->data);
        size_t size = eina_binbuf_length_get(task->data);
        // fprintf(stderr, "Size of data = %d bytes\n", int(size));

        std::vector<char> content(buffer, buffer + size);
        task->cb(std::move(content));
    } else{
        task->cb({});
    }

    return true;
}


bool startUrlRequest(const std::string& _url, UrlCallback _callback) {
    s_urlTaskQueue.push_back(std::make_unique<UrlTask>(_url, _callback));
    return true;

}

void cancelUrlRequest(const std::string& _url) {

    // Only clear this request if a worker has not started operating on it!!
    // otherwise it gets too convoluted with curl!
    // auto itr = s_urlTaskQueue.begin();
    // while(itr != s_urlTaskQueue.end()) {
    //     if((*itr)->url == _url) {
    //         itr = s_urlTaskQueue.erase(itr);
    //     } else {
    //         itr++;
    //     }
    // }
}

static Ecore_Event_Handler *_url_complete_handler = nullptr;
static Ecore_Event_Handler *_url_progress_handler = nullptr;

void initUrlRequests() {
    ecore_con_init();
    _url_complete_handler = ecore_event_handler_add(ECORE_CON_EVENT_URL_COMPLETE,
                                                    completion_callback, nullptr);

    _url_progress_handler = ecore_event_handler_add(ECORE_CON_EVENT_URL_DATA,
                                                    data_callback, nullptr);
}

void finishUrlRequests() {
    ecore_event_handler_del(_url_complete_handler);
    ecore_event_handler_del(_url_progress_handler);
    ecore_con_shutdown();
}
#else

#include "urlWorker.h"

//static std::unique_ptr<UrlWorker> s_workers = nullptr;


static UrlWorker s_workers;

bool startUrlRequest(const std::string& _url, UrlCallback _callback) {
    s_workers.enqueue(std::make_unique<UrlTask>(_url, _callback));
    return true;
}

void cancelUrlRequest(const std::string& _url) {
}

void initUrlRequests() {
    s_workers.start(4);

    //s_workers.reset(workers);
}

void finishUrlRequests() {
    s_workers.stop();
}


#endif


static bool s_update = false;
#ifdef  LOG_TAG
#undef  LOG_TAG
#endif
#define LOG_TAG "Tangram"

void logMsg(const char* fmt, ...) {

    va_list vl;
    va_start(vl, fmt);
    dlog_vprint(DLOG_WARN, LOG_TAG, fmt, vl);
    va_end(vl);
}

void setRenderCallbackFunction(std::function<void()> callback) {
    s_renderCallbackFunction = callback;
}

void setEvasGlAPI(Evas_GL_API *glApi) {
    __evas_gl_glapi = glApi;
}

void requestRender() {
    s_update = true;
    if (s_renderCallbackFunction) {
        s_renderCallbackFunction();
    }
}

bool shouldRender() {
    bool update = s_update;
    s_update = false;
    return update;
}


void setContinuousRendering(bool _isContinuous) {

    s_isContinuousRendering = _isContinuous;

}

bool isContinuousRendering() {

    return s_isContinuousRendering;

}

std::string stringFromFile(const char* _path) {

    size_t length = 0;
    unsigned char* bytes = bytesFromFile(_path, length);

    std::string out(reinterpret_cast<char*>(bytes), length);
    free(bytes);

    return out;
}

void initPlatformFontSetup() {

    static bool s_platformFontsInit = false;
    if (s_platformFontsInit) { return; }

    s_fcConfig = FcInitLoadConfigAndFonts();

    std::string style = "Regular";

    FcStrSet* fcLangs = FcGetLangs();
    FcStrList* fcLangList = FcStrListCreate(fcLangs);
    FcChar8* fcLang;
    while ((fcLang = FcStrListNext(fcLangList))) {
        FcValue fcStyleValue, fcLangValue;

        fcStyleValue.type = fcLangValue.type = FcType::FcTypeString;
        fcStyleValue.u.s = (const FcChar8*)(style.c_str());
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
            if (FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch) {
                // Make sure this font file is not previously added.
                if (std::find(s_fallbackFonts.begin(), s_fallbackFonts.end(), (char*)file) == s_fallbackFonts.end()) {
                    s_fallbackFonts.emplace_back((char*)file);
                }
            }
            FcPatternDestroy(font);
        }
        FcPatternDestroy(pat);
    }
    FcStrListDone(fcLangList);
    s_platformFontsInit = true;
}

std::string systemFontFallbackPath(int _importance, int _weightHint) {

    if ((size_t)_importance >= s_fallbackFonts.size()) {
        return "";
    }

    return s_fallbackFonts[_importance];
}

std::string systemFontPath(const std::string& _name, const std::string& _weight,
                           const std::string& _face) {

    initPlatformFontSetup();

    if (!s_fcConfig) {
        return "";
    }

    std::string fontFile;
    FcValue fcFamily, fcFace, fcWeight;

    fcFamily.type = fcFace.type = fcWeight.type = FcType::FcTypeString;
    fcFamily.u.s = (const FcChar8*)(_name.c_str());
    fcWeight.u.s = (const FcChar8*)(_weight.c_str());
    fcFace.u.s = (const FcChar8*)(_face.c_str());

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
        if (FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch &&
            FcPatternGetString(font, FC_FAMILY, 0, &fontFamily) == FcResultMatch) {
            // We do not want the "best" match, but an "exact" or at least the same "family" match
            // We have fallbacks to cover rest here.
            if (strcmp((char*)fontFamily, _name.c_str()) == 0) {
                fontFile = (char*)file;
            }
        }
        FcPatternDestroy(font);
    } else {
        return "";
    }

    FcPatternDestroy(pattern);

    return fontFile;
}

unsigned char* bytesFromFile(const char* _path, size_t& _size) {

    if (!_path || strlen(_path) == 0) { return nullptr; }

    std::ifstream resource(_path, std::ifstream::ate | std::ifstream::binary);

    if(!resource.is_open()) {
        logMsg("Failed to read file at path: %s\n", _path);
        _size = 0;
        return nullptr;
    }

    _size = resource.tellg();

    resource.seekg(std::ifstream::beg);

    char* cdata = (char*) malloc(sizeof(char) * (_size));

    resource.read(cdata, _size);
    resource.close();

    return reinterpret_cast<unsigned char *>(cdata);
}

void setCurrentThreadPriority(int priority){
    int tid = syscall(SYS_gettid);
    //int  p1 = getpriority(PRIO_PROCESS, tid);

    setpriority(PRIO_PROCESS, tid, priority);

    //int  p2 = getpriority(PRIO_PROCESS, tid);
    //logMsg("set niceness: %d -> %d\n", p1, p2);
}

void initGLExtensions() {
     // glBindVertexArrayOESEXT = (PFNGLBINDVERTEXARRAYPROC)glfwGetProcAddress("glBindVertexArray");
     // glDeleteVertexArraysOESEXT = (PFNGLDELETEVERTEXARRAYSPROC)glfwGetProcAddress("glDeleteVertexArrays");
     // glGenVertexArraysOESEXT = (PFNGLGENVERTEXARRAYSPROC)glfwGetProcAddress("glGenVertexArrays");
}
