#pragma once

#if defined(PLATFORM_RPI) || defined(PLATFORM_LINUX)

#define NUM_WORKERS 3

static UrlWorker s_Workers[NUM_WORKERS];
static std::list<std::unique_ptr<UrlTask>> s_urlTaskQueue;

void logMsg(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}

std::string resolvePath(const char* _path, PathType _type) {
    switch (_type) {
    case PathType::absolute:
    case PathType::internal:
        return std::string(_path);
    case PathType::resource:
        return s_resourceRoot + _path;
    }
    return "";
}

std::string setResourceRoot(const char* _path) {
    std::string dir(_path);

    s_resourceRoot = std::string(dirname(&dir[0])) + '/';

    std::string base(_path);

    return std::string(basename(&base[0]));
}

// No system fonts implementation (yet!)
std::string systemFontPath(const std::string& _name, const std::string& _weight,
                           const std::string& _face) {
    return "";
}

// No system fonts fallback implementation (yet!)
std::string systemFontFallbackPath(int _importance, int _weightHint) {
    return "";
}

void processNetworkQueue() {
    // attach workers to NetWorkerData
    auto taskItr = s_urlTaskQueue.begin();
    for(auto& worker : s_Workers) {
        if(taskItr == s_urlTaskQueue.end()) {
            break;
        }
        if(worker.isAvailable()) {
            worker.perform(std::move(*taskItr));
            taskItr = s_urlTaskQueue.erase(taskItr);
        }
    }
}

std::string stringFromFile(const char* _path, PathType _type) {
    unsigned int length = 0;
    unsigned char* bytes = bytesFromFile(_path, _type, &length);

    std::string out(reinterpret_cast<char*>(bytes), length);
    free(bytes);

    return out;
}

unsigned char* bytesFromFile(const char* _path, PathType _type, unsigned int* _size) {
    std::string path = resolvePath(_path, _type);

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

bool startUrlRequest(const std::string& _url, UrlCallback _callback) {
    std::unique_ptr<UrlTask> task(new UrlTask(_url, _callback));
    for(auto& worker : s_Workers) {
        if(worker.isAvailable()) {
            worker.perform(std::move(task));
            return true;
        }
    }
    s_urlTaskQueue.push_back(std::move(task));
    return true;

}

void cancelUrlRequest(const std::string& _url) {
    // Only clear this request if a worker has not started operating on it!!
    // otherwise it gets too convoluted with curl!
    auto itr = s_urlTaskQueue.begin();
    while(itr != s_urlTaskQueue.end()) {
        if((*itr)->url == _url) {
            itr = s_urlTaskQueue.erase(itr);
        } else {
            itr++;
        }
    }
}

#endif // PLATFORM_RPI || PLATFORM_LINUX

#if defined(PLATFORM_OSX) || defined(PLATFORM_IOS)

static NSMutableString* s_resourceRoot = NULL;

// No system fonts implementation (yet!)
std::string systemFontPath(const std::string& _name, const std::string& _weight, const std::string& _face) {
    return "";
}

// No system fonts fallback implementation (yet!)
std::string systemFontFallbackPath(int _importance, int _weightHint) {
    return "";
}

void logMsg(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}

std::string setResourceRoot(const char* _path) {
    NSString* path = [NSString stringWithUTF8String:_path];

    if (*_path != '/') {
        NSString* resources = [[NSBundle mainBundle] resourcePath];
        path = [resources stringByAppendingPathComponent:path];
    }

    s_resourceRoot = [ [path stringByDeletingLastPathComponent] mutableCopy];

    return std::string([[path lastPathComponent] UTF8String]);
}

NSString* resolvePath(const char* _path, PathType _type) {
    if (s_resourceRoot == NULL) {
        setResourceRoot(".");
    }

    NSString* path = [NSString stringWithUTF8String:_path];

    switch (_type) {
        case PathType::internal:
            return [[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:path];
        case PathType::resource:
            return [s_resourceRoot stringByAppendingPathComponent:path];
        case PathType::absolute:
            return path;
    }
}

void cancelUrlRequest(const std::string& _url) {
    NSString* nsUrl = [NSString stringWithUTF8String:_url.c_str()];

    [defaultSession getTasksWithCompletionHandler:^(NSArray* dataTasks, NSArray* uploadTasks, NSArray* downloadTasks) {
        for(NSURLSessionTask* task in dataTasks) {
            if([[task originalRequest].URL.absoluteString isEqualToString:nsUrl]) {
                [task cancel];
                break;
            }
        }
    }];
}

std::string stringFromFile(const char* _path, PathType _type) {
    NSString* path = resolvePath(_path, _type);
    NSString* str = [NSString stringWithContentsOfFile:path
        usedEncoding:NULL
        error:NULL];

    if (str == nil) {
        logMsg("Failed to read file at path: %s\n", [path UTF8String]);
        return std::string();
    }

    return std::string([str UTF8String]);
}

unsigned char* bytesFromFile(const char* _path, PathType _type, unsigned int* _size) {
    NSString* path = resolvePath(_path, _type);
    NSMutableData* data = [NSMutableData dataWithContentsOfFile:path];

    if (data == nil) {
        logMsg("Failed to read file at path: %s\n", [path UTF8String]);
        *_size = 0;
        return nullptr;
    }

    *_size = data.length;
    unsigned char* ptr = (unsigned char*)malloc(*_size);
    [data getBytes:ptr length:*_size];

    return ptr;
}

void initGLExtensions() {}

#endif // PLATFORM_OSX || PLATFORM_IOS
