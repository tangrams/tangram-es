#include "glfwApp.h"
#include "windowsPlatform.h"
#include "log.h"
#include "map.h"
#include <memory>
#include <signal.h>
#include <stdlib.h>
#include <direct.h> // _getcwd

using namespace Tangram;

int main(int argc, char* argv[]) {

    auto platform = std::make_shared<WindowsPlatform>();

    // Create the windowed app.
    GlfwApp::create(platform, 1024, 768);
    GlfwApp::parseArgs(argc, argv);
		
    // Resolve the input path against the current directory.
    Url baseUrl("file:///");
    char pathBuffer[PATH_MAX] = {0};
    if (_getcwd(pathBuffer, PATH_MAX) != nullptr) {
        baseUrl = Url("file://" + std::string(pathBuffer)).resolved(baseUrl);
    }
		
    LOG("Base URL: %s", baseUrl.string().c_str());
    
    Url sceneUrl = Url(GlfwApp::sceneFile).resolved(baseUrl);
    GlfwApp::sceneFile = sceneUrl.string();

    // Loop until the user closes the window
    GlfwApp::run();

    // Clean up.
    GlfwApp::destroy();

}
