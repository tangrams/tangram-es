#if defined(PLATFORM_RPI) || defined(PLATFORM_LINUX)

#include "urlWorker.h"
#include <curl/curl.h>

static size_t write_data(void *_buffer, size_t _size, size_t _nmemb, void *_dataPtr) {

    const size_t realSize = _size * _nmemb;

    std::stringstream* stream = (std::stringstream*)_dataPtr;

    stream->write((const char*)_buffer, realSize);

    return realSize;
}

UrlWorker::UrlWorker() {
    m_curlHandle = curl_easy_init();
}

UrlWorker::~UrlWorker() {
    // wait for thread to finish
    if (m_future.valid()) { m_future.get(); }

    curl_easy_cleanup(m_curlHandle);
}

void UrlWorker::perform(std::unique_ptr<UrlTask> _task) {

    m_task = std::move(_task);

    m_future = std::async(std::launch::async, [&]() {

        // set up curl to perform fetch
        curl_easy_setopt(m_curlHandle, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(m_curlHandle, CURLOPT_WRITEDATA, &m_stream);
        curl_easy_setopt(m_curlHandle, CURLOPT_URL, m_task->url.c_str());
        curl_easy_setopt(m_curlHandle, CURLOPT_HEADER, 0L);
        curl_easy_setopt(m_curlHandle, CURLOPT_VERBOSE, 0L);
        curl_easy_setopt(m_curlHandle, CURLOPT_ACCEPT_ENCODING, "gzip");

        LOGD("Fetching URL: %s", m_task->url.c_str());

        // Reset stream
        m_stream.seekp(0);

        CURLcode result = curl_easy_perform(m_curlHandle);

        long httpStatusCode = 0;
        curl_easy_getinfo(m_curlHandle, CURLINFO_RESPONSE_CODE, &httpStatusCode);

        if (result == CURLE_OK && httpStatusCode == 200) {
            size_t nBytes = m_stream.tellp();
            m_stream.seekp(0);

            m_task->content.resize(nBytes);
            m_stream.seekg(0);
            m_stream.read(m_task->content.data(), nBytes);
        } else {
            LOGE("curl_easy_perform failed: %s - %d",
                 curl_easy_strerror(result), httpStatusCode);
        }

        m_task->callback(std::move(m_task->content));
        m_task.reset();

        return true;
    });
}

void UrlWorker::reset() {
    m_task.reset();
}

void UrlWorker::join() {
    if (m_future.valid()) {
        m_future.get();
    }
}

bool UrlWorker::hasTask(const std::string& _url) {
    return (m_task && m_task->url == _url);
}

#endif // PLATFORM_LINUX || PLATFORM_RPI
