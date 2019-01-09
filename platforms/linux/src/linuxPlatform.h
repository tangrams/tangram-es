#pragma once

#include <fontconfig/fontconfig.h>
#include <atomic>

#include "platform.h"
#include "urlClient.h"
#include "util/asyncWorker.h"

namespace Tangram {

class LinuxPlatform : public Platform {
public:
    LinuxPlatform();
    explicit LinuxPlatform(UrlClient::Options urlClientOptions);
    ~LinuxPlatform() override;
    void shutdown() override;
    void requestRender() const override;
    std::vector<FontSourceHandle> systemFontFallbacksHandle() const override;
    FontSourceHandle systemFont(const std::string& _name, const std::string& _weight,
                                const std::string& _face) const override;

    UrlRequestId startUrlRequest(Url _url, UrlRequestHandle _handle) override;
    void urlRequestCanceled(UrlRequestId _id) override;

protected:
    FcConfig* m_fcConfig = nullptr;
    std::unique_ptr<UrlClient> m_urlClient;
    AsyncWorker m_fileWorker;

};

} // namespace Tangram
