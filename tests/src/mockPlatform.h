#pragma once

#include "platform.h"

namespace Tangram {

class MockPlatform : public Platform {

public:

    void requestRender() const override;
    std::vector<FontSourceHandle> systemFontFallbacksHandle() const override;
    bool startUrlRequest(const std::string& _url, UrlCallback _callback) override;
    void cancelUrlRequest(const std::string& _url) override;

};

} // namespace Tangram
