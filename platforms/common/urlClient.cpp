#include "urlClient.h"
#include "log.h"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <curl/curl.h>
#include <unistd.h>
#include <time.h>

constexpr char const* requestCancelledError = "Request cancelled";

namespace Tangram {

struct CurlGlobals {
    CurlGlobals() {
        LOGD("curl global init");
        curl_global_init(CURL_GLOBAL_ALL);
    }
    ~CurlGlobals() {
        LOGD("curl global shutdown");
        curl_global_cleanup();
    }
} s_curl;

struct UrlClient::Task {
    // Reduce Task content capacity when it's more than 128kb and last
    // content size was less then half limit_capacity.
    static constexpr size_t limit_capacity = 128 * 1024;

    Request request;
    std::vector<char> content;
    CURL *handle = nullptr;
    char curlErrorString[CURL_ERROR_SIZE] = {0};
    bool active = false;
    bool canceled = false;

    static size_t curlWriteCallback(char* ptr, size_t size, size_t n, void* user) {
        // Writes data received by libCURL.
        auto* task = reinterpret_cast<Task*>(user);
        if (task->canceled) { return 0; }

        auto& buffer = task->content;
        auto addedSize = size * n;
        auto oldSize = buffer.size();
        buffer.resize(oldSize + addedSize);
        std::memcpy(buffer.data() + oldSize, ptr, addedSize);
        return addedSize;
    }

    Task(const Options& _options) {
        // Set up an easy handle for reuse.
        handle = curl_easy_init();
        curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, &curlWriteCallback);
        curl_easy_setopt(handle, CURLOPT_WRITEDATA, this);
        curl_easy_setopt(handle, CURLOPT_NOPROGRESS, 1L);
        curl_easy_setopt(handle, CURLOPT_HEADER, 0L);
        curl_easy_setopt(handle, CURLOPT_VERBOSE, 0L);
        curl_easy_setopt(handle, CURLOPT_ACCEPT_ENCODING, "gzip");
        curl_easy_setopt(handle, CURLOPT_ERRORBUFFER, curlErrorString);
        curl_easy_setopt(handle, CURLOPT_FAILONERROR, 1L);
        curl_easy_setopt(handle, CURLOPT_CONNECTTIMEOUT_MS, _options.connectionTimeoutMs);
        curl_easy_setopt(handle, CURLOPT_TIMEOUT_MS, _options.requestTimeoutMs);
        curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1);
        curl_easy_setopt(handle, CURLOPT_MAXREDIRS, 20);
        curl_easy_setopt(handle, CURLOPT_TCP_NODELAY, 1);
    }

    void setup() {
        canceled = false;
        active = true;
    }

    void clear() {
        bool shrink = content.capacity() > limit_capacity &&
            !content.empty() && content.size() < limit_capacity / 2;

        if (shrink) {
            LOGD("Release content buffer %u / %u", content.size(), content.capacity());
            content.resize(limit_capacity / 2);
            content.shrink_to_fit();
        }
        content.clear();

        active = false;
    }

    ~Task() {
        curl_easy_cleanup(handle);
    }

    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;

};


UrlClient::UrlClient(Options options) : m_options(options) {

    // Start the curl thread
    m_curlHandle = curl_multi_init();
    m_curlWorker = std::make_unique<std::thread>(&UrlClient::curlLoop, this);
    m_curlRunning = true;

    // Init at least one task to avoid checking whether m_tasks is empty in
    // startPendingRequests()
    m_tasks.emplace_back(m_options);

    // Using a pipe to notify select() in curl-thread of new requests..
    // https://www.linuxquestions.org/questions/programming-9/exit-from-blocked-pselect-661200/
    if (pipe(m_requestNotify) < 0) {
        LOGE("Could not initialize select breaker!");
    }
}

UrlClient::~UrlClient() {
    // Cancel all tasks
    {
        std::lock_guard<std::mutex> lock(m_requestMutex);
        // For all requests that have not started, finish them now with a
        // canceled response.
        for (auto& request : m_requests) {
            if (request.callback) {
                UrlResponse response;
                response.error = requestCancelledError;
                request.callback(std::move(response));
            }
        }
        m_requests.clear();
        for (auto& task : m_tasks) {
            task.canceled = true;
        }
    }

    // Stop the curl threads.
    m_curlRunning = false;
    curlWakeUp();

    m_curlWorker->join();

    // 1 - curl_multi_remove_handle before any easy handles are cleaned up
    // 2 - curl_easy_cleanup can now be called independently since the easy handle
    //     is no longer connected to the multi handle
    // 3 - curl_multi_cleanup should be called when all easy handles are removed

    // This is probably not needed since all task have been canceled and joined
    for (auto& task : m_tasks) {
        if (task.active) {
            curl_multi_remove_handle(m_curlHandle, task.handle);
        }
    }
    curl_multi_cleanup(m_curlHandle);
}

void UrlClient::curlWakeUp() {

    if (!m_curlNotified) {
        if (write(m_requestNotify[1], "\0", 1) <= 0) {
            // err
            return;
        }
        //LOG("wake up!");
        m_curlNotified = true;
    }
}

UrlClient::RequestId UrlClient::addRequest(const std::string& _url, UrlCallback _onComplete) {

    auto id = ++m_requestCount;
    Request request = {_url, _onComplete, id};

    // Add the request to our list.
    {
        // Lock the mutex to prevent concurrent modification of the
        // list by the curl thread.
        std::lock_guard<std::mutex> lock(m_requestMutex);
        m_requests.push_back(request);
    }
    curlWakeUp();

    return id;
}

void UrlClient::cancelRequest(UrlClient::RequestId _id) {
    UrlCallback callback;
    // First check the pending request list.
    {
        std::lock_guard<std::mutex> lock(m_requestMutex);
        for (auto it = m_requests.begin(), end = m_requests.end(); it != end; ++it) {
            auto& request = *it;
            if (request.id == _id) {
                callback = std::move(request.callback);
                m_requests.erase(it);
                break;
            }
        }
    }

    // We run the callback outside of the mutex lock to prevent deadlock
    // in case the callback makes further calls into this UrlClient.
    if (callback) {
        UrlResponse response;
        response.error = requestCancelledError;
        callback(std::move(response));
        return;
    }

    // Check requests that are already active.
    {
        std::lock_guard<std::mutex> lock(m_requestMutex);
        auto it = std::find_if(std::begin(m_tasks), std::end(m_tasks),
                               [&](auto& t) { return t.request.id == _id; });

        if (it != std::end(m_tasks)) {
            it->canceled = true;
        }
    }
}

void UrlClient::startPendingRequests() {
    std::unique_lock<std::mutex> lock(m_requestMutex);

    while (m_activeTasks < m_options.maxActiveTasks) {

        if (m_requests.empty()) { break; }

        if (m_tasks.front().active) {
            m_tasks.emplace_front(m_options);
        }

        m_activeTasks++;

        Task& task = m_tasks.front();

        // Swap front with back
        m_tasks.splice(std::end(m_tasks), m_tasks, std::begin(m_tasks));

        task.setup();

        task.request = std::move(m_requests.front());
        m_requests.erase(m_requests.begin());

        // Configure the easy handle.
        const char* url = task.request.url.c_str();
        curl_easy_setopt(task.handle, CURLOPT_URL, url);

        LOGD("Tasks %d - starting request for url: %s", int(m_activeTasks), url);

        curl_multi_add_handle(m_curlHandle, task.handle);
    }
}

void UrlClient::curlLoop() {
    // Based on: https://curl.haxx.se/libcurl/c/multi-app.html

    // Loop until the session is destroyed.
    while (m_curlRunning) {

        fd_set fdread, fdwrite, fdexcep;
        FD_ZERO(&fdread);
        FD_ZERO(&fdwrite);
        FD_ZERO(&fdexcep);

        // 100ms select() default timeout
        struct timeval timeout{0, 100 * 1000};

        int maxfd = -1;
        long curl_timeo = -1;
        curl_multi_timeout(m_curlHandle, &curl_timeo);

        if (curl_timeo >= 0) {
            timeout.tv_usec = 0;
            timeout.tv_sec = curl_timeo / 1000;
            if (timeout.tv_sec > 1) {
                timeout.tv_sec = 1;
            } else {
                timeout.tv_usec = (curl_timeo % 1000) * 1000;
            }
            if (timeout.tv_sec > 0) {
                timeout.tv_sec = 0;
            } else {
                timeout.tv_usec = (curl_timeo % 1000) * 1000;
            }
            //printf ("Timeout %ld.%06ld\n", timeout.tv_sec, timeout.tv_usec);
        }

        // Get file descriptors from the transfers
        CURLMcode mc = curl_multi_fdset(m_curlHandle, &fdread, &fdwrite, &fdexcep,
                                        &maxfd);
        if (mc != CURLM_OK) {
            LOGE("curl_multi_fdset() failed, code %d.", mc);
            continue;
        }

        // Listen on requestNotify to break select when new requests are added.
        FD_SET(m_requestNotify[0], &fdread);

        // Wait for transfers
        // On success the value of maxfd is guaranteed to be >= -1. We call
        // select(maxfd + 1, ...); specially in case of (maxfd == -1) there are
        // no fds ready yet so we call select(0, ...) to sleep 100ms, which is
        // the minimum suggested value in the curl_multi_fdset() doc.
        int ready = select(maxfd + 2, &fdread, &fdwrite, &fdexcep, &timeout);

        if (ready == -1) {
            LOGE("select() error!");
            continue;

        } else {
            if (FD_ISSET(m_requestNotify[0], &fdread)) {
                // Clear notify fd
                char buffer[1];
                int n = read(m_requestNotify[0], buffer, sizeof(buffer));
                if (n <= 0) { LOGE("Read request notify %d", n); }
                //LOG("Got request notifies %d %d", n, m_curlNotified);
                m_curlNotified = false;
            }

            // Create tasks from request queue
            startPendingRequests();

            //
            int activeRequests = 0;
            curl_multi_perform(m_curlHandle, &activeRequests);
        }

        while (true) {
            // how many messages are left
            int msgs_left;
            struct CURLMsg *msg = curl_multi_info_read(m_curlHandle, &msgs_left);
            if (!msg) break;

            //LOG("Messages left: %d / active %d", msgs_left, m_activeTasks);

            // Easy handle must be removed for reuse
            curl_multi_remove_handle(m_curlHandle, msg->easy_handle);

            // NB: DONE is the only defined message type.
            if (msg->msg != CURLMSG_DONE) {
                LOGE("Unhandled curl info msg: %d", msg->msg);
                continue;
            }

            CURLcode resultCode = msg->data.result;
            CURL* handle = msg->easy_handle;

            UrlCallback callback;
            UrlResponse response;
            {
                std::lock_guard<std::mutex> lock(m_requestMutex);
                // Find Task for this message
                auto it = std::find_if(std::begin(m_tasks), std::end(m_tasks),
                                       [&](auto& t) { return t.handle == handle; });
                if (it == std::end(m_tasks)) {
                    assert(false);
                    continue;
                }
                auto& task = *it;
                // Move task to front - for quick reuse
                m_tasks.splice(std::begin(m_tasks), m_tasks, it);

                // Get Response content and Request callback
                callback = std::move(task.request.callback);
                response.content = task.content;

                const char* url = task.request.url.c_str();
                if (resultCode == CURLE_OK) {
                    LOGD("Succeeded for url: %s", url);
                    response.error = nullptr;

                } else if (task.canceled) {
                    LOGD("Aborted request for url: %s", url);
                    response.error = requestCancelledError;

                } else {
                    LOGW("Failed with error for url: %s", task.curlErrorString, url);
                    response.error = task.curlErrorString;
                }

                // Unset task state, clear content
                task.clear();
            }

            // Always run callback regardless of request result.
            if (callback) {
                m_dispatcher.enqueue([callback = std::move(callback),
                                      response = std::move(response)]() mutable {
                                         callback(std::move(response));
                                     });

                //callback(std::move(response));
            }
            m_activeTasks--;
        }
    }
}

} // namespace Tangram
