#pragma once

#include "platform.h"
#include <condition_variable>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace Tangram {

class UrlClient {

public:

    struct Options {
        uint32_t numberOfThreads = 4;
        uint32_t connectionTimeoutMs = 3000;
        uint32_t requestTimeoutMs = 30000;
    };

    UrlClient(Options options);
    ~UrlClient();

    using Callback = std::function<void(UrlResponse&&)>;

    UrlRequestId addRequest(const std::string& url, Callback cb);

    void cancelRequest(UrlRequestId request);

private:

    struct Request {
        std::string url;
        Callback callback;
        UrlRequestId id;
        bool canceled;
    };

    using Response = UrlResponse;

    struct Task {
        Request request;
        std::vector<char> content;
    };

    static Response getCanceledResponse();
    static size_t curlWriteCallback(char* ptr, size_t size, size_t n, void* user);
    static int curlProgressCallback(void* user, double dltotal, double dlnow, double ultotal, double ulnow);

    void curlLoop(uint32_t index);

    std::vector<std::thread> m_threads;
    std::vector<Task> m_tasks;
    std::vector<Request> m_requests;
    std::condition_variable m_requestCondition;
    std::mutex m_requestMutex;
    Options m_options;
    UrlRequestHandle m_requestCount = 0;
    bool m_keepRunning = false;
};

} // namespace Tangram
