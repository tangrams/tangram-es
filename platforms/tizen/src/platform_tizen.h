#pragma once

#include "platform.h"
#include "urlClient.h"

#include <functional>
#include <Evas_GL.h>

namespace Tangram {

void setEvasGlAPI(Evas_GL_API *glApi);

class TizenPlatform : public Platform {

public:

    TizenPlatform();
    TizenPlatform(UrlClient::Options urlClientOptions);
    ~TizenPlatform() override;
    void requestRender() const override;
    bool startUrlRequest(const std::string& _url, UrlCallback _callback) override;
    void cancelUrlRequest(const std::string& _url) override;


    std::vector<FontSourceHandle> systemFontFallbacksHandle() const override;

    std::vector<char> systemFont(const std::string& _name, const std::string& _weight,
                                 const std::string& _face) const override;

    void setRenderCallbackFunction(std::function<void()> callback);

    
protected:

    void initPlatformFontSetup() const;

    std::string fontPath(const std::string& _name, const std::string& _weight,
                         const std::string& _face) const;

    UrlClient m_urlClient;

    std::function<void()> m_renderCallbackFunction = nullptr;

    mutable bool m_update = false;
};

} // namespace Tangram
