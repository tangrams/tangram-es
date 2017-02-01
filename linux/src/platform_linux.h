#pragma once

#include "platform.h"

void processNetworkQueue();

#define NUM_WORKERS 3

class LinuxPlatform : public Platform {

public:

    ~LinuxPlatform() override;
    void requestRender() const override;
    std::vector<FontSourceHandle> systemFontFallbacksHandle() const override;
    bool startUrlRequest(const std::string& _url, UrlCallback _callback) const override;
    void cancelUrlRequest(const std::string& _url) const override;
    void processNetworkQueue();

private:

    UrlWorker m_workers[NUM_WORKERS];
    std::list<std::unique_ptr<UrlTask>> m_urlTaskQueue;

};

