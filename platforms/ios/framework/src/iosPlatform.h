#pragma once

#include "platform.h"

@class TGMapView;

namespace Tangram {

class iOSPlatform : public Platform {

public:

    iOSPlatform(__weak TGMapView* _mapView);
    void shutdown() override {}
    void requestRender() const override;
    void setContinuousRendering(bool _isContinuous) override;
    std::vector<FontSourceHandle> systemFontFallbacksHandle() const override;
    FontSourceHandle systemFont(const std::string& _name, const std::string& _weight, const std::string& _face) const override;
    bool startUrlRequestImpl(const Url& _url, const UrlRequestHandle _request, UrlRequestId& _id) override;
    void cancelUrlRequestImpl(const UrlRequestId _id) override;

private:

    __weak TGMapView* m_mapView;

};

} // namespace Tangram
