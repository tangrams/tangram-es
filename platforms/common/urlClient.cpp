#include "urlClient.h"
#include "log.h"
#include <cassert>
#include <cstring>

namespace Tangram {

UrlClient::Response getCanceledResponse() {
    UrlClient::Response response;
    response.canceled = true;
    return response;
}

UrlClient::UrlClient(Options options) : m_options(options) {
    assert(options.numberOfThreads > 0);
    // Start the url fetching threads.
    m_keepRunning = true;
    m_tasks.resize(options.numberOfThreads);
    for (uint32_t i = 0; i < options.numberOfThreads; i++) {
        m_threads.emplace_back(&UrlClient::fetchLoop, this, i);
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
    // Stop the fetch threads.
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
        // Lock the mutex to prevent concurrent modification of the list by the fetch loop thread.
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

} // namespace Tangram
