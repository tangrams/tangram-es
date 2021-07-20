#include "glfwApp.h"
#include "windowsPlatform.h"
#include "log.h"
#include "map.h"
#include <memory>
#include <direct.h> // _getcwd

#define PATH_MAX 512

using namespace Tangram;

int main(int argc, char* argv[]) {

    // Create the windowed app.
    GlfwApp::create(std::make_unique<WindowsPlatform>(), 1024, 768);

    GlfwApp::sceneFile = "res/scene.yaml";
    GlfwApp::parseArgs(argc, argv);

    // Resolve the input path against the current directory.
    Url baseUrl("file://");
    char pathBuffer[PATH_MAX] = {0};
    if (getcwd(pathBuffer, PATH_MAX) != nullptr) {
        baseUrl = baseUrl.resolve(Url("/" + std::string(pathBuffer) + "/"));
    }

    LOG("Base URL: %s", baseUrl.string().c_str());

    Url sceneUrl = baseUrl.resolve(Url(GlfwApp::sceneFile));
    GlfwApp::sceneFile = sceneUrl.string();

    // Loop until the user closes the window
    GlfwApp::run();

    // Clean up.
    GlfwApp::destroy();

    return 0;
}
