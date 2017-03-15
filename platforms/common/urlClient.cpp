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


UrlClient::Response getCanceledResponse() {
    UrlClient::Response response;
    response.canceled = true;
    return response;
}

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
    // Make all tasks cancelled.
    {
        std::lock_guard<std::mutex> lock(m_requestMutex);
        for (auto& request : m_requests) {
            if (request.callback) {
                auto response = getCanceledResponse();
                request.callback(std::move(response.data));
            }
        }
        m_requests.clear();
        for (auto& task : m_tasks) {
            task.response.canceled = true;
        }
    }
    // Stop the curl threads.
    m_keepRunning = false;
    m_requestCondition.notify_all();
    for (auto& thread : m_threads) {
        thread.join();
    }
}

bool UrlClient::addRequest(const std::string& url, UrlCallback onComplete) {
    // Create a new request.
    Request request = {url, onComplete};
    // Add the request to our list.
    {
        // Lock the mutex to prevent concurrent modification of the list by the curl loop thread.
        std::lock_guard<std::mutex> lock(m_requestMutex);
        m_requests.push_back(request);
    }
    // Notify a thread to start the transfer.
    m_requestCondition.notify_one();
    return true;
}

void UrlClient::cancelRequest(const std::string& url) {
    std::lock_guard<std::mutex> lock(m_requestMutex);
    // First check the pending request list.
    for (auto it = m_requests.begin(), end = m_requests.end(); it != end; ++it) {
        auto& request = *it;
        if (request.url == url) {
            // Found the request! Now run its callback and remove it.
            auto response = getCanceledResponse();
            if (request.callback) {
                request.callback(std::move(response.data));
            }
            m_requests.erase(it);
            return;
        }
    }
    // Next check the active request list.
    for (auto& task : m_tasks) {
        if (task.request.url == url) {
            task.response.canceled = true;
        }
    }
}

size_t curlWriteCallback(char* ptr, size_t size, size_t n, void* user) {
    // Writes data received by libCURL.
    auto* response = reinterpret_cast<UrlClient::Response*>(user);
    auto& buffer = response->data;
    auto addedSize = size * n;
    auto oldSize = buffer.size();
    buffer.resize(oldSize + addedSize);
    std::memcpy(buffer.data() + oldSize, ptr, addedSize);
    return addedSize;
}

int curlProgressCallback(void* user, double dltotal, double dlnow, double ultotal, double ulnow) {
    // Signals libCURL to abort the request if marked as canceled.
    auto* response = reinterpret_cast<UrlClient::Response*>(user);
    return static_cast<int>(response->canceled);
}

void UrlClient::curlLoop(uint32_t index) {
    assert(m_tasks.size() > index);
    Task& task = m_tasks[index];
    LOGD("curlLoop %u starting", index);
    // Create a buffer for curl error messages.
    char curlErrorString[CURL_ERROR_SIZE];
    // Set up an easy handle for reuse.
    auto handle = curl_easy_init();
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, &curlWriteCallback);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, &task.response);
    curl_easy_setopt(handle, CURLOPT_PROGRESSFUNCTION, &curlProgressCallback);
    curl_easy_setopt(handle, CURLOPT_PROGRESSDATA, &task.response);
    curl_easy_setopt(handle, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(handle, CURLOPT_HEADER, 0L);
    curl_easy_setopt(handle, CURLOPT_VERBOSE, 0L);
    curl_easy_setopt(handle, CURLOPT_ACCEPT_ENCODING, "gzip");
    curl_easy_setopt(handle, CURLOPT_ERRORBUFFER, curlErrorString);
    curl_easy_setopt(handle, CURLOPT_CONNECTTIMEOUT_MS, m_options.connectionTimeoutMs);
    curl_easy_setopt(handle, CURLOPT_TIMEOUT_MS, m_options.requestTimeoutMs);
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
            // Configure the easy handle.
            const char* url = task.request.url.data();
            curl_easy_setopt(handle, CURLOPT_URL, url);
            LOGD("curlLoop %u starting request for url: %s", index, url);
            // Perform the request.
            auto result = curl_easy_perform(handle);
            // Get the result status code.
            long httpStatus = 0;
            curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &httpStatus);
            // Handle success or error.
            if (result == CURLE_OK && httpStatus >= 200 && httpStatus < 300) {
                LOGD("curlLoop %u succeeded with http status: %d for url: %s", index, httpStatus, url);
                task.response.successful = true;
            } else if (result == CURLE_ABORTED_BY_CALLBACK) {
                LOGD("curlLoop %u request aborted for url: %s", index, url);
                task.response.successful = false;
            } else {
                LOGE("curlLoop %u failed: '%s' with http status: %d for url: %s", index, curlErrorString, httpStatus, url);
                task.response.successful = false;
            }
            if (task.request.callback) {
                LOGD("curlLoop %u performing request callback", index);
                task.request.callback(std::move(task.response.data));
            }
        }
        // Reset the response.
        task.response.data.clear();
        task.response.canceled = false;
        task.response.successful = false;
    }
    LOGD("curlLoop %u exiting", index);
    // Clean up our easy handle.
    curl_easy_cleanup(handle);
}

} // namespace Tangram
