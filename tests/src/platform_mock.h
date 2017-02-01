#pragma once

#include "platform.h"

class MockPlatform : public Platform {

public:

    void requestRender() const override;
    std::vector<FontSourceHandle> systemFontFallbacksHandle() const override;
    bool startUrlRequest(const std::string& _url, UrlCallback _callback) const override;
    void cancelUrlRequest(const std::string& _url) const override;

};


