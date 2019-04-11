#include "glfwApp.h"
#include "log.h"
#include "map.h"
#include "osxPlatform.h"
#include <memory>
#include <signal.h>
#include <stdlib.h>

#import <AppKit/AppKit.h>

using namespace Tangram;

@interface TGPreferences : NSObject
+ (void)setup;
@end

@implementation TGPreferences

+ (void)setup
{
    // Set up menu shortcuts.
    NSMenu *mainMenu = [[NSApplication sharedApplication] mainMenu];

    // Set up File menu.
    NSMenuItem* fileMenuItem = [mainMenu insertItemWithTitle:@"" action:nil keyEquivalent:@"" atIndex:1];
    NSMenu* fileMenu = [[NSMenu alloc] init];
    [fileMenuItem setSubmenu:fileMenu];
    [fileMenu setTitle:@"File"];
    [fileMenu addItemWithTitle:@"Open Scene with External Editor"
                       action:@selector(startFileEdit)
                keyEquivalent:@"e"].target = self;

    [fileMenu addItemWithTitle:@"Open Scene..."
                       action:@selector(startFileOpen)
                keyEquivalent:@"o"].target = self;

    [fileMenu addItemWithTitle:@"Reload Scene"
                       action:@selector(startFileReload)
                keyEquivalent:@"r"].target = self;
}

+ (void)startFileOpen
{
    NSOpenPanel* openPanel = [NSOpenPanel openPanel];
    openPanel.canChooseFiles = YES;
    openPanel.canChooseDirectories = NO;
    openPanel.allowsMultipleSelection = NO;

    [openPanel beginWithCompletionHandler:^(NSModalResponse result){
        if (result == NSModalResponseOK) {
            NSURL *url = openPanel.URLs.firstObject;
            LOG("Got URL to open: %s", [[url absoluteString] UTF8String]);
            GlfwApp::sceneFile = [[url absoluteString] UTF8String];
            GlfwApp::sceneYaml.clear();
            GlfwApp::loadSceneFile();
        }
    }];
}

+ (void)startFileEdit
{
    NSString* file = [NSString stringWithUTF8String:GlfwApp::sceneFile.c_str()];
    NSURL* url = [NSURL URLWithString:file relativeToURL:[[NSBundle mainBundle] resourceURL]];
    [[NSWorkspace sharedWorkspace] openURL:url];
}

+ (void)startFileReload
{
    GlfwApp::loadSceneFile();
}
@end

int main(int argc, char* argv[]) {

    // Create the windowed app.
    GlfwApp::create(std::make_unique<OSXPlatform>(), 1024, 768);

    // Menu and window changes must come after window is created.
    [TGPreferences setup];

    GlfwApp::parseArgs(argc, argv);

    NSString* sceneInputString = [NSString stringWithUTF8String:GlfwApp::sceneFile.c_str()];
    NSURL* resourceDirectoryUrl = [[NSBundle mainBundle] resourceURL];
    NSURL* sceneFileUrl = [NSURL URLWithString:sceneInputString relativeToURL:resourceDirectoryUrl];
    if (sceneFileUrl == nil) {
        // Parsing input as a URL failed, try as a file path next.
        sceneFileUrl = [NSURL fileURLWithPath:sceneInputString relativeToURL:resourceDirectoryUrl];
    }

    if (sceneFileUrl != nil) {
        GlfwApp::sceneFile = std::string([[sceneFileUrl absoluteString] UTF8String]);
    } else {
        LOGE("Scene input could not be resolved to a valid URL: %s", [sceneInputString UTF8String]);
    }


    // Give it a chance to shutdown cleanly on CTRL-C
    signal(SIGINT, &GlfwApp::stop);

    // Loop until the user closes the window
    GlfwApp::run();

    // Clean up.
    GlfwApp::destroy();
}
