#pragma once

#include "platform.h" // UrlResponse
#include "util/asyncWorker.h"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <list>
#include <mutex>
#include <deque>
#include <string>
#include <thread>
#include <vector>

#if defined(_WIN32)
#include <winsock2.h>
#endif

namespace Tangram {

class UrlClient {

public:

    struct Options {
        uint32_t maxActiveTasks = 20;
        uint32_t connectionTimeoutMs = 3000;
        uint32_t requestTimeoutMs = 30000;
        const char* userAgentString = "tangram";
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

    class SelfPipe {
    public:
#if defined(_WIN32)
        using Socket = SOCKET;
        const Socket SocketInvalid = INVALID_SOCKET;
#else
        using Socket = int;
        const Socket SocketInvalid = -1;
#endif
        ~SelfPipe();
        bool initialize();
        bool write();
        bool read(int *error);
        Socket getReadFd();
    private:
        Socket pipeFds[2] = { SocketInvalid, SocketInvalid };
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
    std::atomic<uint64_t> m_requestCount{0};

    // File descriptors to break waiting select.
    SelfPipe m_requestNotify;
};

} // namespace Tangram
