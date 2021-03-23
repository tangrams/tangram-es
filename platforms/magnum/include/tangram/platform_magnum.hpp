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

    ~MagnumTexture();

private:
    void loadSceneFile(bool setPosition = false, const std::vector<SceneUpdate>& updates = {});

private:
    class Impl;
    Impl* impl_;
};


} // namespace Tangram
