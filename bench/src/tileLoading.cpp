#include "tangram.h"
#include "gl.h"
#include "platform.h"
#include "data/dataSource.h"
#include "scene/sceneLoader.h"
#include "scene/scene.h"
#include "style/style.h"
#include "scene/styleContext.h"
#include "util/mapProjection.h"
#include "tile/tile.h"

#include <vector>
#include <iostream>
#include <fstream>

#include "benchmark/benchmark_api.h"
#include "benchmark/benchmark.h"

using namespace Tangram;

struct TestContext {

    MercatorProjection s_projection;
    const char* sceneFile = "scene.yaml";

    std::unique_ptr<Scene> scene;
    StyleContext styleContext;

    std::shared_ptr<DataSource> source;

    std::vector<char> rawTileData;

    std::shared_ptr<TileData> tileData;

    void loadScene(const char* sceneFile) {
        auto sceneRelPath = setResourceRoot(sceneFile);
        auto sceneString = stringFromFile(sceneRelPath.c_str(), PathType::resource);

        YAML::Node sceneNode;

        try { sceneNode = YAML::Load(sceneString); }
        catch (YAML::ParserException e) {
            LOGE("Parsing scene config '%s'", e.what());
            return;
        }
        scene = std::make_unique<Scene>();
        SceneLoader::loadScene(sceneNode, *scene);
        styleContext.initFunctions(*scene);
        styleContext.setGlobalZoom(0);

        source = scene->dataSources()[0];
    }

    void loadTile(const char* path){
        std::ifstream resource(path, std::ifstream::ate | std::ifstream::binary);
        if(!resource.is_open()) {
            LOGE("Failed to read file at path: %s", path.c_str());
            return;
        }

        size_t _size = resource.tellg();
        resource.seekg(std::ifstream::beg);

        rawTileData.resize(_size);

        resource.read(&rawTileData[0], _size);
        resource.close();
    }

    void parseTile() {
        Tile tile({0,0,0}, s_projection);
        tileData = source->parse(tile, rawTileData);
    }
};

class TileLoadingFixture : public benchmark::Fixture {
public:
    MercatorProjection s_projection;
    const char* sceneFile = "scene.yaml";

    std::unique_ptr<Scene> scene;
    StyleContext styleContext;

    std::shared_ptr<DataSource> source;
    std::shared_ptr<TileData> tileData;
    std::vector<char> rawTileData;

    void SetUp() override {
        LOG("SETUP");
        if (scene) {  return; }

        auto sceneRelPath = setResourceRoot(sceneFile);
        auto sceneString = stringFromFile(sceneRelPath.c_str(), PathType::resource);

        YAML::Node sceneNode;

        try { sceneNode = YAML::Load(sceneString); }
        catch (YAML::ParserException e) {
            LOGE("Parsing scene config '%s'", e.what());
            return;
        }
        scene = std::make_unique<Scene>();
        SceneLoader::loadScene(sceneNode, *scene);
        styleContext.initFunctions(*scene);
        styleContext.setGlobalZoom(10);

        const char* path = "tile.mvt";

        std::ifstream resource(path, std::ifstream::ate | std::ifstream::binary);
        if(!resource.is_open()) {
            LOGE("Failed to read file at path: %s", path.c_str());
            return;
        }

        size_t _size = resource.tellg();
        resource.seekg(std::ifstream::beg);

        rawTileData.resize(_size);

        resource.read(&rawTileData[0], _size);
        resource.close();

        LOG("Initialized TileData");

        Tile tile({0,0,0}, s_projection);

        if (!scene->dataSources().empty()) {
            source = scene->dataSources()[0];
            tileData = source->parse(tile, rawTileData);
        }

        LOG("Ready");
    }
    void TearDown() override {
        LOG("TEARDOWN");
    }
};

BENCHMARK_DEFINE_F(TileLoadingFixture, BuildTest)(benchmark::State& st) {

    while (st.KeepRunning()) {
        if (tileData) {
            Tile tile({0,0,0}, s_projection);
            tile.build(styleContext, *scene, *tileData, *source);
        }
    }
}

BENCHMARK_REGISTER_F(TileLoadingFixture, BuildTest);



BENCHMARK_MAIN();
