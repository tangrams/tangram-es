#include "glfwApp.h"
#include "log.h"
#include "platform_linux.h"
#include "tangram.h"
#include <memory>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

using namespace Tangram;

int main(int argc, char* argv[]) {

    auto platform = std::make_shared<LinuxPlatform>();

    std::string inputString = "scene.yaml";
    // Load file from command line, if given.
    int argi = 0;
    while (++argi < argc) {
        if (strcmp(argv[argi - 1], "-f") == 0) {
            inputString = std::string(argv[argi]);
            LOG("File from command line: %s\n", argv[argi]);
            break;
        }
    }
    // Resolve the input path against the current directory.
    Url baseUrl("file:///");
    char pathBuffer[PATH_MAX] = {0};
    if (getcwd(pathBuffer, PATH_MAX) != nullptr) {
        baseUrl = Url(std::string(pathBuffer) + "/").resolved(baseUrl);
    }

    LOG("Base URL: %s", baseUrl.string().c_str());
    Url sceneUrl = Url(inputString).resolved(baseUrl);

    // Create the windowed app.
    GlfwApp::create(platform, sceneUrl.string(), 1024, 768);

    // Give it a chance to shutdown cleanly on CTRL-C
    signal(SIGINT, &GlfwApp::stop);

    // Loop until the user closes the window
    GlfwApp::run();

    // Clean up.
    GlfwApp::destroy();

}
