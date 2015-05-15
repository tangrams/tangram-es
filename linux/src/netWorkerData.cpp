#include "netWorkerData.h"
#include "platform.h"

#include <cstring>
#include <curl/curl.h>

static size_t write_data(void *_buffer, size_t _size, size_t _nmemb, void *_dataPtr) {
    
    const size_t realSize = _size * _nmemb;

    std::stringstream* stream = (std::stringstream*)_dataPtr;
    
    stream->write((const char*)_buffer, realSize);

    return realSize;
}

NetworkWorker::NetworkWorker() {
    m_curlHandle = curl_easy_init();
}

NetworkWorker::~NetworkWorker() {
    curl_easy_cleanup(m_curlHandle);
} 

void NetworkWorker::perform(std::unique_ptr<NetWorkerData> _workerData) {
    
    m_workerData = std::move(_workerData);
    m_available = false;

    m_future = std::async(std::launch::async, [&]() {

        // set up curl to perform fetch
        curl_easy_setopt(m_curlHandle, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(m_curlHandle, CURLOPT_WRITEDATA, &m_stream);
        curl_easy_setopt(m_curlHandle, CURLOPT_URL, m_workerData->url.c_str());
        curl_easy_setopt(m_curlHandle, CURLOPT_HEADER, 0L);
        curl_easy_setopt(m_curlHandle, CURLOPT_VERBOSE, 0L);
        curl_easy_setopt(m_curlHandle, CURLOPT_ACCEPT_ENCODING, "gzip");
    
        logMsg("Fetching URL with curl: %s\n", m_workerData->url.c_str());

        CURLcode result = curl_easy_perform(m_curlHandle);
        
        if (result != CURLE_OK) {
            logMsg("curl_easy_perform failed: %s\n", curl_easy_strerror(result));
        }

        size_t nBytes = m_stream.tellp();
        m_stream.seekp(0);

        m_workerData->rawData.resize(nBytes);
        m_stream.seekg(0);
        m_stream.read(m_workerData->rawData.data(), nBytes);

        m_finished = true;
        requestRender();
        return std::move(m_workerData);
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

