#pragma once

#include "platform.h"
#include "urlClient.h"

using Tangram::UrlClient;

class LinuxPlatform : public Platform {

public:

    LinuxPlatform();
    LinuxPlatform(UrlClient::Options urlClientOptions);
    ~LinuxPlatform() override;
    void requestRender() const override;
    std::vector<FontSourceHandle> systemFontFallbacksHandle() const override;
    bool startUrlRequest(const std::string& _url, UrlCallback _callback) override;
    void cancelUrlRequest(const std::string& _url) override;
    void processNetworkQueue();

protected:

    UrlClient m_urlClient;

};

