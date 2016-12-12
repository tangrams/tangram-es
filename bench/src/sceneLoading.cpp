#include "tangram.h"
#include "gl.h"
#include "platform.h"
#include "log.h"
#include "scene/sceneLoader.h"
#include "scene/scene.h"
#include "style/style.h"
#include "scene/importer.h"

#include <vector>
#include <iostream>
#include <fstream>
#include <chrono>

#include <unistd.h>
#include <stdio.h>

#include "benchmark/benchmark_api.h"
#include "benchmark/benchmark.h"

using namespace Tangram;

class SceneLoadingFixture : public benchmark::Fixture {
public:
    std::string sceneFile;

    void SetUp() override {
        char path[FILENAME_MAX];
        if (auto workingDir = getcwd(path, sizeof(path))) {
            sceneFile = std::string(workingDir) + "/bubble-wrap/bubble-wrap.yaml";
        }

        if (std::ifstream(sceneFile)) {
            LOG("Testing bubble-wrap.yaml");
        } else {
            if (auto workingDir = getcwd(path, sizeof(path))) {
                sceneFile = std::string(workingDir) + "/scene.yaml";
            }
            if (!std::ifstream(sceneFile)) {
                sceneFile.clear();
            }
        }
    }
    void TearDown() override {
        sceneFile.clear();
    }
};

BENCHMARK_DEFINE_F(SceneLoadingFixture, BuildTest)(benchmark::State& st) {
    if (sceneFile.empty()) { return; }

    while (st.KeepRunning()) {
        std::shared_ptr<Scene> scene = std::make_shared<Scene>(sceneFile);

        clock_t begin = clock();

        Importer sceneImporter;
        scene->config() = sceneImporter.applySceneImports(scene->path(), scene->resourceRoot());

        float loadTime = (float(clock() - begin) / CLOCKS_PER_SEC) * 1000;
        LOG("loadTime %f", loadTime);

    }
}

BENCHMARK_REGISTER_F(SceneLoadingFixture, BuildTest);


BENCHMARK_MAIN();
