#include "log.h"
#include "platform_linux.h"
#include "platform_gl.h"
#include "tangram.h"
#include "headlessContext.h"

#include <memory>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

using namespace Tangram;

int main(int argc, char* argv[]) {

    auto platform = std::make_shared<LinuxPlatform>();

    int width = 800;
    int height = 600;

    std::string sceneFile = "scene.yaml";

    // Load file from command line, if given.
    int argi = 0;
    while (++argi < argc) {
        if (strcmp(argv[argi - 1], "-f") == 0) {
            sceneFile = std::string(argv[argi]);
            LOG("File from command line: %s\n", argv[argi]);
            break;
        }
    }

    signal(SIGINT, [](int){
            logMsg("killed!\n");
            exit(1);
        });

    HeadlessContext ctx;
    ctx.init();
    ctx.resize(width, height);
    ctx.makeCurrent();
    
    // Setup tangram
    Tangram::Map map(platform);

    map.setupGL();

    //map.setPixelScale(density);
    map.resize(width, height);

    const std::string& apiKey = "vector-tiles-tyHL4AY";

    map.loadScene(sceneFile.c_str(),
                  false,
                  {SceneUpdate("global.sdk_mapzen_api_key", apiKey)});


    while (!map.update(1000)) {
        usleep(10000);
    }

    LOG("render!");

    map.render();

    GL::finish();

}
