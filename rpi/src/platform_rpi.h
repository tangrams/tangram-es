#pragma once

#include "platform.h"

void processNetworkQueue();

#define NUM_WORKERS 3

class RPiPlatform : public Platform {

public:

    ~RPiPlatform() override;
    void requestRender() const override;
    std::vector<FontSourceHandle> systemFontFallbacksHandle() const override;
    bool startUrlRequest(const std::string& _url, UrlCallback _callback) override;
    void cancelUrlRequest(const std::string& _url) override;
    void processNetworkQueue();

private:

    mutable UrlWorker m_workers[NUM_WORKERS];
    mutable std::list<std::unique_ptr<UrlTask>> m_urlTaskQueue;

};

