#include "glfwApp.h"
#include "linuxPlatform.h"
#include "log.h"
#include "map.h"
#include <memory>
#include <limits.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

using namespace Tangram;

int main(int argc, char* argv[]) {

    // Create the windowed app.
    GlfwApp::create(std::make_unique<LinuxPlatform>(), 1024, 768);

    GlfwApp::sceneFile = "res/scene.yaml";
    GlfwApp::parseArgs(argc, argv);

    // Resolve the input path against the current directory.
    Url baseUrl("file:///");
    char pathBuffer[PATH_MAX] = {0};
    if (getcwd(pathBuffer, PATH_MAX) != nullptr) {
        baseUrl = Url(std::string(pathBuffer) + "/").resolved(baseUrl);
    }
    
    LOG("Base URL: %s", baseUrl.string().c_str());
    
    Url sceneUrl = Url(GlfwApp::sceneFile).resolved(baseUrl);
    GlfwApp::sceneFile = sceneUrl.string();

    // Give it a chance to shutdown cleanly on CTRL-C
    signal(SIGINT, &GlfwApp::stop);

    // Loop until the user closes the window
    GlfwApp::run();

    // Clean up.
    GlfwApp::destroy();

}
