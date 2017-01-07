#pragma once

#include "platform.h"
#include "urlClient.h"

namespace Tangram {

class WindowsPlatform : public Platform {

public:

    WindowsPlatform();
    WindowsPlatform(UrlClient::Options urlClientOptions);
    ~WindowsPlatform() override;
    void requestRender() const override;
    std::vector<FontSourceHandle> systemFontFallbacksHandle() const override;
    bool startUrlRequest(const std::string& _url, UrlCallback _callback) override;
    void cancelUrlRequest(const std::string& _url) override;
    std::string resourceRoot() const override;

protected:

    UrlClient m_urlClient;

};

} // namespace Tangram
