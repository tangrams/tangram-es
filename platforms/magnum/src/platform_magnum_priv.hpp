#pragma once
#include "platform.h"
#include "urlClient.h"
namespace Tangram {
class PlatformMagnum : public Platform {
public:
    explicit PlatformMagnum(uint32_t maxActiveTasks = 20, uint32_t connectionTimeoutMs = 3000,
                            uint32_t requestTimeoutMs = 30000);
    ~PlatformMagnum() override;
    void shutdown() override;
    void requestRender() const override;

    bool startUrlRequestImpl(const Url& _url, const UrlRequestHandle _request, UrlRequestId& _id) override;
    void cancelUrlRequestImpl(const UrlRequestId _id) override;

    bool isDirty() const;
    void setDirty(bool dirty);

private:
    std::unique_ptr<UrlClient> url_client_;
    mutable bool needs_render_;
};
} // namespace Tangram
