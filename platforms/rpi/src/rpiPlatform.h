#pragma once

#include <fontconfig/fontconfig.h>

#include "platform.h"
#include "urlClient.h"

namespace Tangram {

class RpiPlatform : public Platform {

public:

    RpiPlatform();
    RpiPlatform(UrlClient::Options urlClientOptions);
    ~RpiPlatform() override;
    void requestRender() const override;
    std::vector<FontSourceHandle> systemFontFallbacksHandle() const override;
    UrlRequestHandle startUrlRequest(Url _url, UrlCallback _callback) override;
    void cancelUrlRequest(UrlRequestHandle _url) override;
    FontSourceHandle systemFont(const std::string& _name, const std::string& _weight,
            const std::string& _face) const override;

protected:

    FcConfig* m_fcConfig = nullptr;
    UrlClient m_urlClient;

};

} // namespace Tangram
