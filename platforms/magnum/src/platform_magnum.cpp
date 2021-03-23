#include "platform_magnum.hpp"
#include "gl/hardware.h"
#include "log.h"
#include "urlClient.h"
#include <Magnum/GL/Context.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/RenderbufferFormat.h>
#include <Magnum/GL/TextureFormat.h>
#include <dirent.h>
#include <filesystem>
#include <stdarg.h>
#include <stdio.h>
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
MagnumTexture::MagnumTexture(uint32_t maxActiveTasks, uint32_t connectionTimeoutMs, uint32_t requestTimeoutMs)
    : msaa_level_{4}, scene_size_{500, 500}, framebuffer_{{{}, scene_size_}}, renderbuffer_{{{}, scene_size_}},
      need_scene_reload_{false} {
    create(maxActiveTasks, connectionTimeoutMs, requestTimeoutMs);
}

void MagnumTexture::render(const double time) {
    loadSceneFile();

    auto& platform = static_cast<PlatformMagnum&>(map_->getPlatform());
    if (platform.isDirty()) {


        platform.setDirty(false);
        const double delta = time - last_time_;
        last_time_ = time;

        MapState state = map_->update(static_cast<float>(delta));


        framebuffer_.clear(Magnum::GL::FramebufferClear::Color | Magnum::GL::FramebufferClear::Depth).bind();

        Magnum::GL::Context::current().resetState(Magnum::GL::Context::State::EnterExternal);
        map_->render();
        Magnum::GL::Context::current().resetState(Magnum::GL::Context::State::ExitExternal);

        renderbuffer_.clear(Magnum::GL::FramebufferClear::Color).bind();
        Magnum::GL::Framebuffer::blit(framebuffer_, renderbuffer_, {{}, scene_size_},
                                      Magnum::GL::FramebufferBlit::Color);

        Magnum::GL::defaultFramebuffer.bind();
    }
}

Magnum::GL::Texture2D& MagnumTexture::texture() { return render_texture_; }

void MagnumTexture::create(uint32_t maxActiveTasks, uint32_t connectionTimeoutMs, uint32_t requestTimeoutMs) {
    char* nextzenApiKeyEnvVar = getenv("NEXTZEN_API_KEY");
    if (nextzenApiKeyEnvVar && strlen(nextzenApiKeyEnvVar) > 0) {
        apiKey = nextzenApiKeyEnvVar;
    } else {
        LOGW("No API key found!\n\nNextzen data sources require an API key. "
             "Sign up for a key at https://developers.nextzen.org/about.html and then set it from the command line "
             "with: "
             "\n\n\texport NEXTZEN_API_KEY=YOUR_KEY_HERE"
             "\n\nOr, if using an IDE on macOS, with: "
             "\n\n\tlaunchctl setenv NEXTZEN_API_KEY YOUR_API_KEY\n");
    }

    const char* apiKeyScenePath = "global.sdk_api_key";

    if (!apiKey.empty()) { sceneUpdates.push_back(SceneUpdate(apiKeyScenePath, apiKey)); }

    createBuffers();

    sceneFile = "scene.yaml";
    {
        Url baseUrl("file:///");
        LOG("curr URL: %s", std::filesystem::current_path().generic_string().c_str());
        baseUrl = baseUrl.resolve(Url(std::filesystem::current_path().generic_string() + "/"));

        LOG("Base URL: %s", baseUrl.string().c_str());

        Url sceneUrl = baseUrl.resolve(Url(sceneFile));
        sceneFile = sceneUrl.string();
        LOG("Scene URL: %s", sceneFile.c_str());
    }

    sceneFile = "file://D:/dev/tangram-test/build/scene.yaml";


    map_ = std::make_unique<Tangram::Map>(
        std::make_unique<PlatformMagnum>(maxActiveTasks, connectionTimeoutMs, requestTimeoutMs));


    map_->setupGL();

    need_scene_reload_ = true;
}

void MagnumTexture::createBuffers() {
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

void MagnumTexture::loadSceneFile(bool setPosition, const std::vector<SceneUpdate>& updates) {
    for (auto& update : updates) {
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

    if (need_scene_reload_) {
        need_scene_reload_ = false;
        bool load_async = false;
        bool setPosition = true;
        if (load_async) {
            if (!sceneYaml.empty()) {
                map_->loadSceneYamlAsync(sceneYaml, sceneFile, setPosition, sceneUpdates);
            } else {
                map_->loadSceneAsync(sceneFile, setPosition, sceneUpdates);
            }
        } else {
            if (!sceneYaml.empty()) {
                map_->loadSceneYaml(sceneYaml, sceneFile, setPosition, sceneUpdates);
            } else {
                map_->loadScene(sceneFile, setPosition, sceneUpdates);
            }
        }
    }
}

} // namespace Tangram
