#pragma once

#include <fontconfig/fontconfig.h>

#include "platform.h"
#include "urlClient.h"

namespace Tangram {

class RpiPlatform : public Platform {

public:

    RpiPlatform();
    explicit RpiPlatform(UrlClient::Options urlClientOptions);
    ~RpiPlatform() override;
    void requestRender() const override;
    std::vector<FontSourceHandle> systemFontFallbacksHandle() const override;
    FontSourceHandle systemFont(const std::string& _name, const std::string& _weight,
            const std::string& _face) const override;

    bool startUrlRequestImpl(const Url& _url, const UrlRequestHandle _request, UrlRequestId& _id) override;
    void cancelUrlRequestImpl(const UrlRequestId _id) override;

protected:

    FcConfig* m_fcConfig = nullptr;
    UrlClient m_urlClient;

};

} // namespace Tangram
