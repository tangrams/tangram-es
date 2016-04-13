#include <stdio.h>
#include <stdarg.h>
#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <memory>
#include <algorithm>

//#include "urlWorker.h"
#include "platform_tizen.h"

#include <app_common.h>
#include <dlog.h>

#include <libgen.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/syscall.h>


#define NUM_WORKERS 4

static bool s_isContinuousRendering = false;
static std::string s_resourceRoot;


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
//    va_list args;
//    va_start(args, fmt);
//    vfprintf(stderr, fmt, args);
//    va_end(args);

        va_list vl;
        va_start(vl, fmt);
        dlog_vprint(DLOG_WARN, LOG_TAG, fmt, vl);
        va_end(vl);
}

void requestRender() {
    s_update = true;
    //glfwPostEmptyEvent();
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

std::string setResourceRoot(const char* _path) {

    //std::string dir(_path);
    //s_resourceRoot = std::string(dirname(&dir[0])) + '/';
    char *app_res = app_get_resource_path();
    s_resourceRoot = std::string(app_res);
    free(app_res);

    std::string base(_path);

    return std::string(basename(&base[0]));

}

std::string resolvePath(const char* _path, PathType _type) {
        LOG("RESOLVE PATH %s %d", _path, static_cast<int>(_type));
    switch (_type) {
    case PathType::absolute:
    case PathType::internal:
        //return std::string(_path);
        return s_resourceRoot + _path;
    case PathType::resource:
        return s_resourceRoot + _path;
    }
    return "";
}

std::string stringFromFile(const char* _path, PathType _type) {

        //LOG("LOAD STRING %s %d", _path, static_cast<int>(_type));

    unsigned int length = 0;
    unsigned char* bytes = bytesFromFile(_path, _type, &length);

    std::string out(reinterpret_cast<char*>(bytes), length);
    free(bytes);

    return out;
}

// No system fonts fallback implementation (yet!)
std::string systemFontFallbackPath(int _importance, int _weightHint) {
    return "";
}

unsigned char* bytesFromFile(const char* _path, PathType _type, unsigned int* _size) {

        if (!_path || strlen(_path) == 0) { return nullptr; }

    std::string path = resolvePath(_path, _type);

    LOG("LOAD BYTES %s %d", path.c_str(), static_cast<int>(_type));

    std::ifstream resource(path.c_str(), std::ifstream::ate | std::ifstream::binary);

    if(!resource.is_open()) {
        logMsg("Failed to read file at path: %s\n", path.c_str());
        *_size = 0;
        return nullptr;
    }

    *_size = resource.tellg();

    resource.seekg(std::ifstream::beg);

    char* cdata = (char*) malloc(sizeof(char) * (*_size));

    resource.read(cdata, *_size);
    resource.close();

    return reinterpret_cast<unsigned char *>(cdata);
}

// No system fonts implementation (yet!)
std::string systemFontPath(const std::string& _name, const std::string& _weight,
                           const std::string& _face) {
    return "";
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
