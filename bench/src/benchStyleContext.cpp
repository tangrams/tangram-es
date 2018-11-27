#include "benchmark/benchmark.h"

#include "data/tileData.h"
#include "js/JavaScript.h"
#include "scene/styleContext.h"

#include "mockPlatform.h"
#include "log.h"
#include "data/tileSource.h"
#include "scene/filters.h"
#include "scene/importer.h"
#include "scene/scene.h"
#include "scene/dataLayer.h"
#include "scene/sceneLayer.h"
#include "scene/sceneLoader.h"
#include "text/fontContext.h"
#include "tile/tile.h"
#include "tile/tileTask.h"
#include "util/yamlUtil.h"

#include <functional>

#define ITERATIONS ->Iterations(10)
//#define ITERATIONS

#define RUN(FIXTURE, NAME)                                              \
    BENCHMARK_DEFINE_F(FIXTURE, NAME)(benchmark::State& st) { while (st.KeepRunning()) { run(); } } \
    BENCHMARK_REGISTER_F(FIXTURE, NAME) ITERATIONS;

using namespace Tangram;

template<class Context>
class GetPropertyFixtureFixture : public benchmark::Fixture {
public:
    Context ctx;
    Feature feature;
    void SetUp(const ::benchmark::State& state) override {
        JavaScriptScope<Context> jsScope(ctx);
        ctx.setGlobalValue("language", jsScope.newString("en"));
        feature.props.set("name:en", "Ozymandias");
        feature.props.set("title", "King of Kings");
        feature.props.set("number", 17);
        ctx.setCurrentFeature(&feature);
        ctx.setFunction(0, "function () { return (language && feature['name:' + language]) || title; }");
        ctx.setFunction(1, "function () { return feature.order || feature.number; }");
    }
    __attribute__ ((noinline)) void run() {
        StyleParam::Value value;
        benchmark::DoNotOptimize(value);
        JavaScriptScope<Context> jsScope(ctx);
        value = jsScope.getFunctionResult(0).toString();
        value = (float)jsScope.getFunctionResult(1).toDouble();
     }
};

#ifdef TANGRAM_USE_JSCORE
using JSCoreGetPropertyFixture = GetPropertyFixtureFixture<JSCore::Context>;
RUN(JSCoreGetPropertyFixture, JSCoreGetPropertyBench)
#endif

using DuktapeGetPropertyFixture = GetPropertyFixtureFixture<Duktape::Context>;
RUN(DuktapeGetPropertyFixture, DuktapeGetPropertyBench)

const char scene_file[] = "bubble-wrap-style.zip";
//const char scene_file[] = "res/scene.yaml";
const char tile_file[] = "res/tile.mvt";

std::shared_ptr<Scene> scene;
std::shared_ptr<TileSource> source;
std::shared_ptr<TileData> tileData;

void globalSetup() {
    static std::atomic<bool> initialized{false};
    if (initialized.exchange(true)) { return; }

    std::shared_ptr<MockPlatform> platform = std::make_shared<MockPlatform>();
    Url sceneUrl(scene_file);
    platform->putMockUrlContents(sceneUrl, MockPlatform::getBytesFromFile(scene_file));
    scene = std::make_shared<Scene>(platform, sceneUrl);
    Importer importer(scene);
    try {
        scene->config() = importer.applySceneImports(platform);
    }
    catch (const YAML::ParserException& e) {
        LOGE("Parsing scene config '%s'", e.what());
        exit(-1);
    }
    if (!scene->config()) {
        LOGE("Invalid scene file '%s'", scene_file);
        exit(-1);
    }
    SceneLoader::applyConfig(platform, scene);
    scene->fontContext()->loadFonts();
    for (auto& s : scene->tileSources()) {
        source = s;
        if (source->generateGeometry()) { break; }
    }
    Tile tile({0,0,10,10});
    auto task = source->createTask(tile.getID());
    auto& t = dynamic_cast<BinaryTileTask&>(*task);

    auto rawTileData = MockPlatform::getBytesFromFile(tile_file);
    t.rawTileData = std::make_shared<std::vector<char>>(rawTileData);
    tileData = source->parse(*task);
    if (!tileData) {
        LOGE("Invalid tile file '%s'", tile_file);
        exit(-1);
    }
}

template<size_t jsCore>
struct JSTileStyleFnFixture : public benchmark::Fixture {
    std::unique_ptr<StyleContext> ctx;
    Feature feature;
    uint32_t numFunctions = 0;
    uint32_t evalCnt = 0;

    void SetUp(const ::benchmark::State& state) override {
        globalSetup();
        ctx.reset(new StyleContext(jsCore));
        ctx->initFunctions(*scene);
        ctx->setFilterKey(Filter::Key::zoom, 10);
    }
    void TearDown(const ::benchmark::State& state) override {
        LOG(">>> %d", evalCnt);
    }

    StyleParam::Value styleValue;

    __attribute__ ((noinline))
    void filter(const SceneLayer& layer) {
        if (layer.filter().eval(feature, *ctx)) {
            for (auto& r : layer.rules()) {
                for (auto& sp : r.parameters) {
                    if (sp.function >= 0) {
                        evalCnt++;
                        ctx->evalStyle(sp.function, sp.key, styleValue);
                    }
                }
            }
            for (const auto& sublayer : layer.sublayers()) {
                filter(sublayer);
            }
        }
    }

    __attribute__ ((noinline)) void run() {
        benchmark::DoNotOptimize(styleValue);

        for (const auto& datalayer : scene->layers()) {
            for (const auto& collection : tileData->layers) {
                if (!collection.name.empty()) {
                    const auto& dlc = datalayer.collections();
                    bool layerContainsCollection =
                        std::find(dlc.begin(), dlc.end(), collection.name) != dlc.end();

                    if (!layerContainsCollection) { continue; }
                }

                for (const auto& feat : collection.features) {
                    ctx->setFeature(feat);

                    filter(datalayer);
                }
            }
        }
    }
};

#ifdef TANGRAM_USE_JSCORE
using JSCoreTileStyleFnFixture = JSTileStyleFnFixture<1>;
RUN(JSCoreTileStyleFnFixture, JSCoreTileStyleFnBench)
#endif

using DuktapeTileStyleFnFixture = JSTileStyleFnFixture<0>;
RUN(DuktapeTileStyleFnFixture, DuktapeTileStyleFnBench)


class DirectGetPropertyFixture : public benchmark::Fixture {
public:
    Feature feature;
    void SetUp(const ::benchmark::State& state) override {
        feature.props.set("message", "Hello World!");
    }
    void TearDown(const ::benchmark::State& state) override {}
};
BENCHMARK_DEFINE_F(DirectGetPropertyFixture, DirectGetPropertyBench)(benchmark::State& st) {
    StyleParam::Value value;

    while (st.KeepRunning()) {
        const auto v = feature.props.get("message");
        if (v.is<std::string>()) {
            value = v.get<std::string>();
        } else {
            printf("eerrr\n");
        }
    }
}
BENCHMARK_REGISTER_F(DirectGetPropertyFixture, DirectGetPropertyBench) ITERATIONS;

BENCHMARK_MAIN();
