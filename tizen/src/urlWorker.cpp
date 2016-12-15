#include "urlWorker.h"
#include "log.h"

#include <curl/curl.h>
#include <sstream>

static size_t write_data(void *_buffer, size_t _size, size_t _nmemb, void *_dataPtr) {
        size_t realSize = _size * _nmemb;
        auto* stream = (std::stringstream*)_dataPtr;
    stream->write(reinterpret_cast<const char*>(_buffer), realSize);
    return realSize;
}

void UrlWorker::start(int _numWorker, const char* _proxyAddress) {
    if (m_running) { return; }
    m_running = true;
    m_proxyAddress = _proxyAddress;

    curl_global_init(CURL_GLOBAL_ALL);

    m_workers.reserve(_numWorker);

    for (int i = 0; i < _numWorker; i++) {
        m_workers.emplace_back();
        m_workers.back().thread.reset(new std::thread(&UrlWorker::run, this,
                                                      &m_workers.back()));
    }
}

UrlWorker::~UrlWorker() {
    if (m_running) {
        stop();
    }
}

void UrlWorker::run(Thread* thread) {
    std::stringstream stream;
    CURL* curlHandle;

    {
        std::lock_guard<std::mutex> lock(m_mutexInitCurl);
        curlHandle = curl_easy_init();
        // set up curl to perform fetch
        curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, &stream);
        if (!m_proxyAddress.empty()) {
            curl_easy_setopt(curlHandle, CURLOPT_PROXY, m_proxyAddress.c_str());
        }
        curl_easy_setopt(curlHandle, CURLOPT_HEADER, 0L);
        curl_easy_setopt(curlHandle, CURLOPT_VERBOSE, 0L);
        curl_easy_setopt(curlHandle, CURLOPT_ACCEPT_ENCODING, "gzip");
    }

    while (true) {

        std::unique_ptr<UrlTask> task;
        {
            std::unique_lock<std::mutex> lock(m_mutex);

            m_condition.wait(lock, [&]{ return !m_running || !m_queue.empty(); });

            // Check if thread should stop
            if (!m_running) { break; }

            task = std::move(m_queue.front());
            m_queue.erase(m_queue.begin());

            if (task) {
                thread->activeUrl = task->url;
                thread->canceled = false;
            }
        }

        if (!task) { continue; }

        LOGD("Fetching URL: %s", task->url.c_str());
        curl_easy_setopt(curlHandle, CURLOPT_URL, task->url.c_str());

        // Reset stream
        stream.seekp(0);
        CURLcode result = curl_easy_perform(curlHandle);

        if (thread->canceled) {
            continue;
        }

        long httpStatusCode = 0;
        curl_easy_getinfo(curlHandle, CURLINFO_RESPONSE_CODE, &httpStatusCode);

        std::vector<char> content;

        if (result == CURLE_OK && httpStatusCode == 200) {
            size_t nBytes = stream.tellp();
            stream.seekp(0);

            content.resize(nBytes);
            stream.seekg(0);
            stream.read(content.data(), nBytes);
        } else {
            LOGE("curl_easy_perform failed: %s - %d",
                   curl_easy_strerror(result), httpStatusCode);
        }

        task->callback(std::move(content));
    }

    curl_easy_cleanup(curlHandle);
}

void UrlWorker::enqueue(std::unique_ptr<UrlTask> _task) {
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (!m_running) {
            return;
        }
        m_queue.push_back(std::move(_task));
    }
    m_condition.notify_one();
}

void UrlWorker::cancel(const std::string& _url) {
    std::unique_lock<std::mutex> lock(m_mutex);
    if (!m_running) {
        return;
    }

    for (auto& task : m_queue) {
        if (task && task->url == _url) {
            task.reset();
        }
    }
    for (auto& worker : m_workers) {
        if (worker.activeUrl == _url) {
            worker.canceled = true;
        }
    }
}

void UrlWorker::stop() {
        bool isRunning;
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        isRunning = m_running;
        m_running = false;
    }

    if (!isRunning) { return; }

    m_condition.notify_all();

    for (auto& worker : m_workers) {
        worker.thread->join();
    }

    m_workers.clear();
    m_queue.clear();

    curl_global_cleanup();
}
