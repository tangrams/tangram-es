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

    struct Response {
        std::vector<char> data;
        bool successful = false;
        bool canceled = false;
    };

    UrlClient(Options options);
    ~UrlClient();

    bool addRequest(const std::string& url, UrlCallback onComplete);

    void cancelRequest(const std::string& url);

private:
    struct Request {
        std::string url;
        UrlCallback callback;
    };

    struct Task {
        Request request;
        Response response;
    };

    void curlLoop(uint32_t index);

    std::vector<std::thread> m_threads;
    std::vector<Task> m_tasks;
    std::vector<Request> m_requests;
    std::condition_variable m_requestCondition;
    std::mutex m_requestMutex;
    Options m_options;
    bool m_keepRunning = false;
};

} // namespace Tangram
