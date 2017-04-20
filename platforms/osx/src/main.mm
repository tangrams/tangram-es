#include "glfwApp.h"
#include "log.h"
#include "platform_osx.h"
#include "tangram.h"
#include <memory>
#include <signal.h>
#include <stdlib.h>

using namespace Tangram;

int main(int argc, char* argv[]) {

    auto platform = std::make_shared<OSXPlatform>();

    NSString* sceneInputString = @"scene.yaml";

    // Load file from command line, if given.
    int argi = 0;
    while (++argi < argc) {
        if (strcmp(argv[argi - 1], "-f") == 0) {
            sceneInputString = [NSString stringWithUTF8String:argv[argi]];
            LOG("File from command line: %s\n", argv[argi]);
            break;
        }
    }

    NSURL* resourceDirectoryUrl = [[NSBundle mainBundle] resourceURL];
    NSURL* sceneFileUrl = [NSURL URLWithString:sceneInputString relativeToURL:resourceDirectoryUrl];

    std::string sceneFile([[sceneFileUrl absoluteString] UTF8String]);

    // Create the windowed app.
    GlfwApp::create(platform, sceneFile, 1024, 768);

    // Give it a chance to shutdown cleanly on CTRL-C
    signal(SIGINT, &GlfwApp::stop);

    // Loop until the user closes the window
    GlfwApp::run();

    // Clean up.
    GlfwApp::destroy();

}
