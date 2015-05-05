#include "netWorkerData.h"
#include "platform.h"

#include <cstring>
#include <curl/curl.h>

static size_t write_data(void *_buffer, size_t _size, size_t _nmemb, void *_dataPtr) {
    const size_t realSize = _size * _nmemb;
    if(realSize < 1) return 0;

    std::vector<char>* data = (std::vector<char>*)_dataPtr;
    if(!data) return 0;

    const size_t prev_total_size = data->size();
    const size_t next_total_size = prev_total_size + realSize;
    
    // Note: Possibilly wasteful ... do better!
    data->resize(next_total_size, 0);
    if(data->size() != next_total_size) return 0;

    memcpy(&((*data)[prev_total_size]), _buffer, realSize);
    return realSize;
}

void NetworkWorker::perform(std::unique_ptr<NetWorkerData> _workerData) {

    m_workerData = std::move(_workerData);
    m_available = false;

    m_future = std::async(std::launch::async, [&]() {

        std::vector<char> rawData;
        CURL* curlHandle = curl_easy_init();

        // set up curl to perform fetch
        curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, &rawData);
        curl_easy_setopt(curlHandle, CURLOPT_URL, m_workerData->url.c_str());
        curl_easy_setopt(curlHandle, CURLOPT_HEADER, 0L);
        curl_easy_setopt(curlHandle, CURLOPT_VERBOSE, 0L);
        curl_easy_setopt(curlHandle, CURLOPT_ACCEPT_ENCODING, "gzip");
    
        logMsg("Fetching URL with curl: %s\n", m_workerData->url.c_str());

        CURLcode result = curl_easy_perform(curlHandle);
        
        curl_easy_cleanup(curlHandle);
        if (result != CURLE_OK) {
            logMsg("curl_easy_perform failed: %s\n", curl_easy_strerror(result));
            rawData.clear();
            m_workerData->setRawData(std::move(rawData));
            m_finished = true;
            requestRender();
            return std::move(m_workerData);
        } else {
            m_workerData->setRawData(std::move(rawData));
            m_finished = true;
            requestRender();
            return std::move(m_workerData);
        }
    });
}

void NetworkWorker::reset() {
    m_workerData.reset();
    m_available = true;
    m_finished = false;
}

bool NetworkWorker::hasWorkerData(const std::string& _url) {
    return (m_workerData->url == _url);
}

std::unique_ptr<NetWorkerData> NetworkWorker::getWorkerResult() {
    return std::move( m_future.get() );
}

