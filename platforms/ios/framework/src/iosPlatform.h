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
    UrlRequestHandle startUrlRequest(Url _url, UrlCallback _callback) override;
    void cancelUrlRequest(UrlRequestHandle _request) override;

private:

    __weak TGMapView* m_mapView;

};

} // namespace Tangram
