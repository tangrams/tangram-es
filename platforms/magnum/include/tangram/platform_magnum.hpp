#pragma once
#include "map.h"
#include "platform.h"
#include <Magnum/GL/Texture.h>
#include <memory>
#include <tangram_export.h>

namespace Tangram {

void TANGRAM_EXPORT setContext(Magnum::GL::Context& ctx);

class TANGRAM_EXPORT MagnumTexture {
public:
    explicit MagnumTexture(uint32_t maxActiveTasks = 20, uint32_t connectionTimeoutMs = 3000,
                           uint32_t requestTimeoutMs = 30000);
    Magnum::GL::Texture2D& texture();
    void render(const double time);
    void setApiKeyFromEnv(const std::string& env_name, const std::string& scene_key);
    void updateApiKey();

    void setSceneFile(const std::string& scene_file);

    ~MagnumTexture();
private:
    class Impl;
    Impl* impl_;
};


} // namespace Tangram
