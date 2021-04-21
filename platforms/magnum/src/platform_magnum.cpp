#include "platform_magnum.hpp"
#include "platform_magnum_priv.hpp"

#include "gl/hardware.h"
#include "log.h"
#include "urlClient.h"
#include <Magnum/GL/Context.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Framebuffer.h>
#include <Magnum/GL/Renderbuffer.h>
#include <Magnum/GL/RenderbufferFormat.h>
#include <Magnum/GL/TextureFormat.h>
#include <cstdarg>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <mutex>
namespace Tangram {

void setContext(Magnum::GL::Context& ctx) {
    flextGLInit(ctx);
    Magnum::GL::Context::makeCurrent(&ctx);
}

void logMsg(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}

void setCurrentThreadPriority(int priority) {}

void initGLExtensions() {
    // Tangram::Hardware::supportsMapBuffer = true;
}

/////////// PlatformMagnum
PlatformMagnum::PlatformMagnum(uint32_t maxActiveTasks, uint32_t connectionTimeoutMs, uint32_t requestTimeoutMs)
    : url_client_{
          std::make_unique<UrlClient>(UrlClient::Options{maxActiveTasks, connectionTimeoutMs, requestTimeoutMs})} {}
PlatformMagnum::~PlatformMagnum() {}
void PlatformMagnum::shutdown() { Platform::shutdown(); }
void PlatformMagnum::requestRender() const { needs_render_ = true; }

bool PlatformMagnum::startUrlRequestImpl(const Url& _url, const UrlRequestHandle _request, UrlRequestId& _id) {

    auto onURLResponse = [this, _request](UrlResponse&& response) { onUrlResponse(_request, std::move(response)); };
    _id = url_client_->addRequest(_url.string(), onURLResponse);
    return false;

    _id = url_client_->addRequest(
        _url.string(), [this, _request](UrlResponse&& response) { onUrlResponse(_request, std::move(response)); });
    return true;
}
void PlatformMagnum::cancelUrlRequestImpl(const UrlRequestId _id) {
    if (url_client_) { url_client_->cancelRequest(_id); }
}

bool PlatformMagnum::isDirty() const { return needs_render_; }
void PlatformMagnum::setDirty(bool dirty) { needs_render_ = dirty; }

/////////// MagnumTexture

class MagnumTexture::Impl {
public:
    Impl(const int width, const int height, const std::string& scene_file, const std::string& api_env_name,
         const std::string& api_env_scene_key, uint32_t maxActiveTasks, uint32_t connectionTimeoutMs,
         uint32_t requestTimeoutMs)
        : msaa_level_{4}, scene_size_{width, height}, framebuffer_{{{}, scene_size_}}, renderbuffer_{{{}, scene_size_}},
          need_scene_reload_{false}, env_name_{api_env_name}, scene_env_key_{api_env_scene_key}, is_panning_{false} {
        setSceneFile(scene_file);
        createBuffers();
        create(maxActiveTasks, connectionTimeoutMs, requestTimeoutMs);
    }

private:
    void create(uint32_t maxActiveTasks, uint32_t connectionTimeoutMs, uint32_t requestTimeoutMs) {

        const auto api_update = updateApiKey();
        if (!api_update.value.empty()) { sceneUpdates.push_back(api_update); }

#if 0
        scene_file_ = "scene.yaml";
        {
            Url baseUrl("file:///");
            LOG("curr URL: %s", std::filesystem::current_path().generic_string().c_str());
            baseUrl = baseUrl.resolve(Url(std::filesystem::current_path().generic_string() + "/"));

            LOG("Base URL: %s", baseUrl.string().c_str());

            Url sceneUrl = baseUrl.resolve(Url(scene_file_));
            scene_file_ = sceneUrl.string();
            LOG("Scene URL: %s", scene_file_.c_str());
        }
#endif
        map_ = std::make_unique<Tangram::Map>(
            std::make_unique<PlatformMagnum>(maxActiveTasks, connectionTimeoutMs, requestTimeoutMs));
        Magnum::GL::Context::current().resetState(Magnum::GL::Context::State::EnterExternal);
        map_->setupGL();
        Magnum::GL::Context::current().resetState(Magnum::GL::Context::State::ExitExternal);

        map_->resize(scene_size_.x(), scene_size_.y());


#if 0
        auto id = map_->markerAdd();
        if (id > 0) {
            LOG("add marker");
            map_->markerSetStylingFromPath(id, "layers.pick-result.draw.pick-marker");
            map_->markerSetPoint(id, LngLat{-40.8481358, -33.6079875});


            CameraPosition pos{};
            pos.latitude = -33.6079875;
            pos.longitude = -40.8481358;
            pos.zoom = 16;
            map_->setCameraPositionEased(pos, 10000);
        }
#endif

        need_scene_reload_ = true;
    }

    void setSceneFile(const std::string& scene_file) {
        scene_file_ = scene_file;
        need_scene_reload_ = true;
    }

    void setSzeneSize(const int width, const int height, bool force = false) {
        if (force || (width != scene_size_.x() || height != scene_size_.y())) {
            scene_size_ = Magnum::Vector2i{width, height};
            createBuffers();
            if (map_) { map_->resize(width, height); }
        }
    }

    void createBuffers() {
        using namespace Magnum;
        framebuffer_ = GL::Framebuffer{{{}, scene_size_}};
        renderbuffer_ = GL::Framebuffer{{{}, scene_size_}};

        render_texture_.setStorage(1, GL::TextureFormat::RGBA8, scene_size_);
        renderbuffer_.attachTexture(GL::Framebuffer::ColorAttachment{0}, render_texture_, 0);

        color_.setStorageMultisample(msaa_level_, GL::RenderbufferFormat::RGBA8, scene_size_);
        depth_stencil_.setStorageMultisample(msaa_level_, GL::RenderbufferFormat::Depth24Stencil8, scene_size_);

        framebuffer_.attachRenderbuffer(GL::Framebuffer::ColorAttachment{0}, color_);
        framebuffer_.attachRenderbuffer(GL::Framebuffer::BufferAttachment::DepthStencil, depth_stencil_);
    }
    void loadSceneFile(bool setPosition = false, bool load_async = false) {
        {
            std::unique_lock l{scene_update_mtx_};
            for (auto& update : scene_updates_to_apply_) {
                bool found = false;
                for (auto& prev : sceneUpdates) {
                    if (update.path == prev.path) {
                        prev = update;
                        found = true;
                        break;
                    }
                }
                if (!found) { sceneUpdates.push_back(update); }
            }
            scene_updates_to_apply_.clear();
        }

        if (need_scene_reload_) {
            need_scene_reload_ = false;
            if (load_async) {
                if (!scene_yaml_.empty()) {
                    map_->loadSceneYamlAsync(scene_yaml_, scene_file_, setPosition, sceneUpdates);
                } else {
                    map_->loadSceneAsync(scene_file_, setPosition, sceneUpdates);
                }
            } else {
                if (!scene_yaml_.empty()) {
                    map_->loadSceneYaml(scene_yaml_, scene_file_, setPosition, sceneUpdates);
                } else {
                    map_->loadScene(scene_file_, setPosition, sceneUpdates);
                }
            }
        }
    }

    SceneUpdate updateApiKey(bool apply_update = false) {
        char* api_key_env_var = std::getenv(env_name_.c_str());
        std::string api_key = "";
        if (api_key_env_var && strlen(api_key_env_var) > 0) {
            api_key = api_key_env_var;
        } else {
            LOGW("No API key found!\n\n");
        }
        if (!api_key.empty()) {
            const SceneUpdate update{scene_env_key_, api_key};

            if (apply_update) {
                std::unique_lock l{scene_update_mtx_};
                scene_updates_to_apply_.push_back(update);
                need_scene_reload_ = true;
            }

            return update;
        }
        return SceneUpdate{};
    }

    void handleStartPan(const double x, const double y) {
        last_click_x_ = x;
        last_click_y_ = y;
        is_panning_ = true;
        LOG("drag start");
    }
    void handlePan(const double x, const double y) {
        if (!is_panning_) return;

        LOG("drag %f %f %f %f", last_click_x_, last_click_y_, x, y);
        map_->handlePanGesture(last_click_x_, last_click_y_, x, y);

        last_click_x_ = x;
        last_click_y_ = y;
    }
    void handlePanEnd() { is_panning_ = false; }

private:
    friend MagnumTexture;
    std::string env_name_;
    std::string scene_env_key_;
    std::string scene_yaml_;
    std::string scene_file_;
    std::mutex scene_update_mtx_;
    std::vector<SceneUpdate> sceneUpdates;
    std::vector<SceneUpdate> scene_updates_to_apply_;
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

    bool is_panning_;
    double last_click_x_;
    double last_click_y_;
};


MagnumTexture::MagnumTexture(const int width, const int height, const std::string& scene_file,
                             const std::string& api_env_name, const std::string& api_env_scene_key,
                             uint32_t maxActiveTasks, uint32_t connectionTimeoutMs, uint32_t requestTimeoutMs)
    : impl_{new Impl{width, height, scene_file, api_env_name, api_env_scene_key, maxActiveTasks, connectionTimeoutMs,
                     requestTimeoutMs}} {}

void MagnumTexture::render(const double time) {
    impl_->loadSceneFile(true, false);

    auto& platform = static_cast<PlatformMagnum&>(impl_->map_->getPlatform());
    if (platform.isDirty()) {
        platform.setDirty(false);
        const double delta = time - impl_->last_time_;
        impl_->last_time_ = time;


        MapState state = impl_->map_->update(static_cast<float>(delta));


        impl_->framebuffer_.clear(Magnum::GL::FramebufferClear::Color | Magnum::GL::FramebufferClear::Depth).bind();

        Magnum::GL::Context::current().resetState(Magnum::GL::Context::State::EnterExternal);
        impl_->map_->render();
        Magnum::GL::Context::current().resetState(Magnum::GL::Context::State::ExitExternal);

        impl_->renderbuffer_.clear(Magnum::GL::FramebufferClear::Color).bind();
        Magnum::GL::Framebuffer::blit(impl_->framebuffer_, impl_->renderbuffer_, {{}, impl_->scene_size_},
                                      Magnum::GL::FramebufferBlit::Color);

        Magnum::GL::defaultFramebuffer.bind();
    }
}

void MagnumTexture::setApiKeyFromEnv(const std::string& env_name, const std::string& scene_key) {
    impl_->env_name_ = env_name;
    impl_->scene_env_key_ = scene_key;
}

void MagnumTexture::resizeScene(const int width, const int height) { impl_->setSzeneSize(width, height); }
void MagnumTexture::handleClick(const double x, const double y) {}
void MagnumTexture::handleDoubleClick(const double x, const double y) {}
void MagnumTexture::handleStartDrag(const double x, const double y) { impl_->handleStartPan(x, y); }
void MagnumTexture::handleDrag(const double x, const double y) { impl_->handlePan(x, y); }
void MagnumTexture::handleEndDrag() { impl_->handlePanEnd(); }

void MagnumTexture::updateApiKey() { impl_->updateApiKey(true); }
void MagnumTexture::setSceneFile(const std::string& scene_file) { impl_->setSceneFile(scene_file); }
Magnum::GL::Texture2D& MagnumTexture::texture() { return impl_->render_texture_; }
std::pair<int, int> MagnumTexture::getSize() const { return {impl_->scene_size_.x(), impl_->scene_size_.y()}; }
void MagnumTexture::zoomDelta(float factor) {
    impl_->map_->setZoom(static_cast<int>(impl_->map_->getZoom() + factor));
    // impl_->map_->setZoom(14.f);
    impl_->map_->setPosition(8.800900262, 53.080924466);
}
MagnumTexture::~MagnumTexture() { delete impl_; }

} // namespace Tangram
