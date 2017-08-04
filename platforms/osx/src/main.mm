#include "glfwApp.h"
#include "log.h"
#include "map.h"
#include "osxPlatform.h"
#include <memory>
#include <signal.h>
#include <stdlib.h>

using namespace Tangram;

int main(int argc, char* argv[]) {

    auto platform = std::make_shared<OSXPlatform>();

    // Create the windowed app.
    GlfwApp::create(platform, 1024, 768);

    GlfwApp::parseArgs(argc, argv);

    // Give it a chance to shutdown cleanly on CTRL-C
    signal(SIGINT, &GlfwApp::stop);

    // Loop until the user closes the window
    GlfwApp::run();

    // Clean up.
    GlfwApp::destroy();

}
