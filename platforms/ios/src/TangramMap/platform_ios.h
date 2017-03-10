#pragma once

#include "platform.h"

#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>

#ifdef __OBJC__
#import "TGMapViewController.h"

namespace Tangram {

class iOSPlatform : public Platform {

public:

    iOSPlatform(TGMapViewController* _viewController);
    void requestRender() const override;
    void setContinuousRendering(bool _isContinuous) override;
    std::string resolveAssetPath(const std::string& _path) const override;
    std::string stringFromFile(const char* _path) const override;
    std::vector<char> bytesFromFile(const char* _path) const override;
    std::vector<FontSourceHandle> systemFontFallbacksHandle() const override;
    std::vector<char> systemFont(const std::string& _name, const std::string& _weight, const std::string& _face) const override;
    bool startUrlRequest(const std::string& _url, UrlCallback _callback) override;
    void cancelUrlRequest(const std::string& _url) override;

private:

    TGMapViewController* m_viewController;

};

} // namespace Tangram

#endif

