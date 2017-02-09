#pragma once

#include "platform.h"
#include "urlWorker.h"
#include <list>

void processNetworkQueue();

#define NUM_WORKERS 3

class LinuxPlatform : public Platform {

public:

    ~LinuxPlatform() override;
    void requestRender() const override;
    std::vector<FontSourceHandle> systemFontFallbacksHandle() const override;
    bool startUrlRequest(const std::string& _url, UrlCallback _callback) override;
    void cancelUrlRequest(const std::string& _url) override;
    void processNetworkQueue();

private:

    UrlWorker m_workers[NUM_WORKERS];
    std::list<std::unique_ptr<UrlTask>> m_urlTaskQueue;

};

