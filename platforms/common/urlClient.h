#pragma once

#include "platform.h" // UrlResponse
#include "util/asyncWorker.h"

#include <condition_variable>
#include <functional>
#include <list>
#include <mutex>
#include <deque>
#include <string>
#include <thread>
#include <vector>


namespace Tangram {

class UrlClient {

public:

    struct Options {
        uint32_t maxActiveTasks = 20;
        uint32_t connectionTimeoutMs = 3000;
        uint32_t requestTimeoutMs = 30000;
    };

    UrlClient(Options options);
    ~UrlClient();

    using RequestId = uint64_t;

    RequestId addRequest(const std::string& url, UrlCallback cb);

    void cancelRequest(RequestId request);

private:

    struct Request {
        std::string url;
        UrlCallback callback;
        RequestId id;
    };

    struct Task;

    void curlLoop();
    void curlWakeUp();

    void startPendingRequests();

    Options m_options;

    // Curl multi handle
    void *m_curlHandle = nullptr;

    bool m_curlRunning = false;
    bool m_curlNotified = false;

    std::unique_ptr<std::thread> m_curlWorker;
    AsyncWorker m_dispatcher;

    std::list<Task> m_tasks;
    uint32_t m_activeTasks = 0;

    std::deque<Request> m_requests;

    // Synchronize m_tasks and m_requests
    std::mutex m_requestMutex;

    // RequestIds
    std::atomic_uint64_t m_requestCount{0};

    // File descriptors to break waiting select.
    int m_requestNotify[2] = { -1, -1 };
};

} // namespace Tangram
