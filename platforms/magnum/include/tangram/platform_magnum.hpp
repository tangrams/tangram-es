#pragma once
#include "map.h"
#include "platform.h"
#include <Magnum/GL/Framebuffer.h>
#include <Magnum/GL/Renderbuffer.h>
#include <Magnum/GL/Texture.h>
#include <memory>
#include <tangram_export.h>

namespace Tangram {

class UrlClient;

void TANGRAM_EXPORT setContext(Magnum::GL::Context& ctx);

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

class TANGRAM_EXPORT MagnumTexture {
public:
    explicit MagnumTexture(uint32_t maxActiveTasks = 20, uint32_t connectionTimeoutMs = 3000,
                           uint32_t requestTimeoutMs = 30000);
    Magnum::GL::Texture2D& texture();
    void render(const double time);

private:
    void update();
    void createBuffers();
    void create(uint32_t maxActiveTasks, uint32_t connectionTimeoutMs, uint32_t requestTimeoutMs);

private:
    std::string apiKey;
    std::string sceneYaml;
    std::string sceneFile;
    std::vector<SceneUpdate> sceneUpdates;
    std::unique_ptr<Tangram::Map> map_;
    double last_time_;
    bool need_scene_reload_;

    int msaa_level_;
    Magnum::Vector2i scene_size_;
    Magnum::GL::Framebuffer framebuffer_;
    Magnum::GL::Framebuffer renderbuffer_;
    Magnum::GL::Renderbuffer color_;
    Magnum::GL::Renderbuffer depth_stencil_;
    Magnum::GL::Texture2D render_texture_;
};


} // namespace Tangram
