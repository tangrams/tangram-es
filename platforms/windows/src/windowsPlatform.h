#pragma once

#include "platform.h"
#include "urlClient.h"
#include "util/asyncWorker.h"

namespace Tangram {

class WindowsPlatform : public Platform {

public:

    WindowsPlatform();
    explicit WindowsPlatform(UrlClient::Options urlClientOptions);
    ~WindowsPlatform() override;
    void shutdown() override;
    void requestRender() const override;
    std::vector<FontSourceHandle> systemFontFallbacksHandle() const override;
    bool startUrlRequestImpl(const Url& _url, const UrlRequestHandle _request, UrlRequestId& _id) override;
    void cancelUrlRequestImpl(const UrlRequestId _id) override;

protected:
    std::unique_ptr<UrlClient> m_urlClient;
    AsyncWorker m_fileWorker;
};

} // namespace Tangram
