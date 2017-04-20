#pragma once

#include "platform.h"

#ifdef __OBJC__
#import "TGMapViewController.h"

namespace Tangram {

class iOSPlatform : public Platform {

public:

    iOSPlatform(TGMapViewController* _viewController);
    void requestRender() const override;
    void setContinuousRendering(bool _isContinuous) override;
    std::vector<FontSourceHandle> systemFontFallbacksHandle() const override;
    std::vector<char> systemFont(const std::string& _name, const std::string& _weight, const std::string& _face) const override;
    UrlRequestHandle startUrlRequest(Url _url, UrlCallback _callback) override;
    void cancelUrlRequest(UrlRequestHandle _request) override;

private:

    TGMapViewController* m_viewController;

};

} // namespace Tangram

#endif

