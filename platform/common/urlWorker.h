#pragma once

#if defined(PLATFORM_RPI) || defined(PLATFORM_LINUX)

#include <future>
#include <memory>
#include <vector>
#include <sstream>

#include "platform.h"

typedef void CURL;

struct UrlTask {
    UrlCallback callback;
    const std::string url;
    std::vector<char> content;

    UrlTask(UrlTask&& _other) :
        callback(std::move(_other.callback)),
        url(std::move(_other.url)),
        content(std::move(_other.content)) {
    }

    UrlTask(const std::string& _url, const UrlCallback& _callback) :
        callback(_callback),
        url(_url) {
    }
};

class UrlWorker {
    public:
        void perform(std::unique_ptr<UrlTask> _task);
        void reset();
        bool isAvailable() { return !bool(m_task); }
        bool hasTask(const std::string& _url);
        void join();

        UrlWorker();
        ~UrlWorker();

    private:
        std::unique_ptr<UrlTask> m_task;
        std::stringstream m_stream;
        CURL* m_curlHandle = nullptr;

        std::future<bool> m_future;
};

#endif
