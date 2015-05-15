#pragma once

#include <future>
#include <memory>
#include <vector>
#include <sstream>

#include "tileID.h"

#define NUM_WORKERS 3
typedef void CURL;

struct NetWorkerData {
    const TileID tileID;
    const std::string url;
    const int dataSourceID;
    std::vector<char> rawData;

    NetWorkerData() : tileID(NOT_A_TILE), url(""), dataSourceID(-1) {
    }

    NetWorkerData(NetWorkerData&& _other) : 
        tileID(std::move(_other.tileID)),
        url(std::move(_other.url)),
        dataSourceID(std::move(_other.dataSourceID)),
        rawData(std::move(_other.rawData)) {
    }

    NetWorkerData(const std::string& _url, const TileID& _tileID, const int& _dataSourceID) : 
        tileID(_tileID),
        url(_url),
        dataSourceID(_dataSourceID) {
    }

    bool operator==(NetWorkerData _other) {
        return (url == _other.url);
    }

    void setRawData(std::vector<char>&& _rawData) {
        rawData = std::move(_rawData);
    }
};

class NetworkWorker {
    public:
        void perform(std::unique_ptr<NetWorkerData> _workerData);
        void reset();
        bool isAvailable() { return m_available; }
        bool isFinished() { return m_finished; }
        bool hasWorkerData(const std::string& _url);
        std::unique_ptr<NetWorkerData> getWorkerResult();

        NetworkWorker();
        ~NetworkWorker();

    private:
        std::unique_ptr<NetWorkerData> m_workerData;
        std::stringstream m_stream;
        bool m_available = true;
        bool m_finished = false;
        CURL* m_curlHandle = nullptr;

        std::future< std::unique_ptr<NetWorkerData> > m_future;
};

