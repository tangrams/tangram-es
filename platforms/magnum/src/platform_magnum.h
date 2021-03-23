#pragma once

#include <atomic>

#include "platform.h"
#include "urlClient.h"

namespace Tangram {

class MagnumPlatform : public Platform {
public:
    MagnumPlatform();
    explicit MagnumPlatform(UrlClient::Options urlClientOptions);
    ~MagnumPlatform() override;
    void shutdown() override;
    void requestRender() const override;
    std::vector<FontSourceHandle> systemFontFallbacksHandle() const override;
    FontSourceHandle systemFont(const std::string& _name, const std::string& _weight,
                                const std::string& _face) const override;

    bool startUrlRequestImpl(const Url& _url, const UrlRequestHandle _request, UrlRequestId& _id) override;
    void cancelUrlRequestImpl(const UrlRequestId _id) override;

protected:
    std::unique_ptr<UrlClient> m_urlClient;
};

} // namespace Tangram
