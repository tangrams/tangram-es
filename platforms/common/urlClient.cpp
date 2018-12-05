#include "urlClient.h"
#include "log.h"
#include <cassert>
#include <cstring>
#include <curl/curl.h>

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

const char* requestCancelledError = "Request cancelled";

UrlClient::UrlClient(Options options) : m_options(options) {
    assert(options.numberOfThreads > 0);
    // Start the curl threads.
    m_keepRunning = true;
    m_tasks.resize(options.numberOfThreads);
    for (uint32_t i = 0; i < options.numberOfThreads; i++) {
        m_threads.emplace_back(&UrlClient::curlLoop, this, i);
    }
}

UrlClient::~UrlClient() {
    // Make all tasks canceled.
    {
        // Lock the mutex to prevent concurrent modification of the list by the curl loop thread.
        std::lock_guard<std::mutex> lock(m_requestMutex);
        // For all requests that have not started, finish them now with a canceled response.
        for (auto& request : m_requests) {
            if (request.callback) {
                request.callback(getCanceledResponse());
            }
        }
        m_requests.clear();
        for (auto& task : m_tasks) {
            task.request.canceled = true;
        }
    }
    // Stop the curl threads.
    m_keepRunning = false;
    m_requestCondition.notify_all();
    for (auto& thread : m_threads) {
        thread.join();
    }
}

UrlClient::RequestId UrlClient::addRequest(const std::string& url, UrlCallback onComplete) {
    // Create a new request.
    auto id = ++m_requestCount;
    Request request = {url, onComplete, id, false};
    // Add the request to our list.
    {
        // Lock the mutex to prevent concurrent modification of the list by the curl loop thread.
        std::lock_guard<std::mutex> lock(m_requestMutex);
        m_requests.push_back(request);
    }
    // Notify a thread to start the transfer.
    m_requestCondition.notify_one();
    return id;
}

void UrlClient::cancelRequest(UrlClient::RequestId id) {
    UrlCallback callback;
    // First check the pending request list.
    {
        // Lock the mutex to prevent concurrent modification of the list by the curl loop thread.
        std::lock_guard<std::mutex> lock(m_requestMutex);
        for (auto it = m_requests.begin(), end = m_requests.end(); it != end; ++it) {
            auto& request = *it;
            if (request.id == id) {
                // Found the request! Now run its callback and remove it.
                callback = std::move(request.callback);
                m_requests.erase(it);
                break;
            }
        }
    }
    // We run the callback outside of the mutex lock to prevent deadlock in case the callback
    // makes further calls into this UrlClient.
    if (callback) {
        callback(getCanceledResponse());
    }

    // Next check the active request list.
    for (auto& task : m_tasks) {
        if (task.request.id == id) {
            task.request.canceled = true;
        }
    }
}

UrlClient::Response UrlClient::getCanceledResponse() {
    UrlClient::Response response;
    response.error = requestCancelledError;
    return response;
}

size_t UrlClient::curlWriteCallback(char* ptr, size_t size, size_t n, void* user) {
    // Writes data received by libCURL.
    auto* response = reinterpret_cast<UrlClient::Response*>(user);
    auto& buffer = response->content;
    auto addedSize = size * n;
    auto oldSize = buffer.size();
    buffer.resize(oldSize + addedSize);
    std::memcpy(buffer.data() + oldSize, ptr, addedSize);
    return addedSize;
}

int UrlClient::curlProgressCallback(void* user, double dltotal, double dlnow, double ultotal, double ulnow) {
    // Signals libCURL to abort the request if marked as canceled.
    auto* task = reinterpret_cast<UrlClient::Task*>(user);
    return static_cast<int>(task->request.canceled);
}

void UrlClient::curlLoop(uint32_t index) {
    assert(m_tasks.size() > index);
    Task& task = m_tasks[index];
    LOGD("curlLoop %u starting", index);
    // Create a buffer for curl error messages.
    char curlErrorString[CURL_ERROR_SIZE] = {0};
    // Set up an easy handle for reuse.
    auto handle = curl_easy_init();
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, &curlWriteCallback);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, &task.content);
    curl_easy_setopt(handle, CURLOPT_PROGRESSFUNCTION, &curlProgressCallback);
    curl_easy_setopt(handle, CURLOPT_PROGRESSDATA, &task);
    curl_easy_setopt(handle, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(handle, CURLOPT_HEADER, 0L);
    curl_easy_setopt(handle, CURLOPT_VERBOSE, 0L);
    curl_easy_setopt(handle, CURLOPT_ACCEPT_ENCODING, "gzip");
    curl_easy_setopt(handle, CURLOPT_ERRORBUFFER, curlErrorString);
    curl_easy_setopt(handle, CURLOPT_FAILONERROR, 1L);
    curl_easy_setopt(handle, CURLOPT_CONNECTTIMEOUT_MS, m_options.connectionTimeoutMs);
    curl_easy_setopt(handle, CURLOPT_TIMEOUT_MS, m_options.requestTimeoutMs);
    curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(handle, CURLOPT_MAXREDIRS, 20);
    curl_easy_setopt(handle, CURLOPT_TCP_NODELAY, 1);

    // Loop until the session is destroyed.
    while (m_keepRunning) {
        bool haveRequest = false;
        // Wait until the condition variable is notified.
        {
            std::unique_lock<std::mutex> lock(m_requestMutex);
            if (m_requests.empty()) {
                LOGD("curlLoop %u waiting", index);
                m_requestCondition.wait(lock);
            }
            LOGD("curlLoop %u notified", index);
            // Try to get a request from the list.
            if (!m_requests.empty()) {
                // Take the first request from our list.
                task.request = m_requests.front();
                m_requests.erase(m_requests.begin());
                haveRequest = true;
            }
        }
        if (haveRequest) {
            const char* url = task.request.url.data();
            LOGTInit("[%u] Starting request: %s", index, url);
            if (task.request.canceled) {
                LOGT("[%u] -------------- canceled -----------", index);
            }

            UrlResponse response;

            // Configure the easy handle.
            curl_easy_setopt(handle, CURLOPT_URL, url);
            // Perform the request.
            auto result = curl_easy_perform(handle);
            // Handle success or error.
            if (result == CURLE_OK) {
                LOGD("curlLoop %u succeeded for url: %s", index, url);
                response.error = nullptr;
            } else if (result == CURLE_ABORTED_BY_CALLBACK) {
                LOGD("curlLoop %u aborted request for url: %s", index, url);
                response.error = requestCancelledError;
            } else {
                LOGD("curlLoop %u failed with error '%s' for url: %s", index, curlErrorString, url);
                response.error = curlErrorString;
            }
            // If a callback is given, always run it regardless of request result.
           if (task.request.canceled) {
               LOGT("[%u] >>>-------------- canceled -----------", index);
            }
            if (task.request.callback) {
                LOGT("[%u] Finished request", index);
                response.content = task.content;
                task.request.callback(std::move(response));
            }
        }
        // Reset the task.
        task.content.clear();
    }
    LOGD("curlLoop %u exiting", index);
    // Clean up our easy handle.
    curl_easy_cleanup(handle);
}

} // namespace Tangram
