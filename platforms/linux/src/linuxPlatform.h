#pragma once

#include <fontconfig/fontconfig.h>

#include "platform.h"
#include "urlClient.h"

namespace Tangram {

class LinuxPlatform : public Platform {

public:

    LinuxPlatform();
    LinuxPlatform(UrlClient::Options urlClientOptions);
    ~LinuxPlatform() override;
    void requestRender() const override;
    std::vector<FontSourceHandle> systemFontFallbacksHandle() const override;
    FontSourceHandle systemFont(const std::string& _name, const std::string& _weight,
            const std::string& _face) const override;
    UrlRequestHandle startUrlRequest(Url _url, UrlCallback _callback) override;
    void cancelUrlRequest(UrlRequestHandle _request) override;

protected:


    FcConfig* m_fcConfig = nullptr;
    UrlClient m_urlClient;

};

} // namespace Tangram
