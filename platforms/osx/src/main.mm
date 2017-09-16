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
+ (void)startApiKeyInput;
+ (void)startFileOpen;
+ (void)startFileEdit;
+ (NSString*) apiKeyDefaultsName;
@end

@implementation TGPreferences

+ (void)setup
{
    // If an API key has been stored in defaults already, set it in the app.
    NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
    NSString* storedApiKey = [defaults stringForKey:[self apiKeyDefaultsName]];
    if (storedApiKey != nil) {
        GlfwApp::mapzenApiKey = std::string([storedApiKey UTF8String]);
    }

    // Set up menu shortcuts.
    NSMenu *mainMenu = [[NSApplication sharedApplication] mainMenu];
    NSMenu *appMenu = [[mainMenu itemAtIndex:0] submenu];
    NSMenuItem* apiKeyMenuItem = [appMenu insertItemWithTitle:@"API Key..."
                                                       action:@selector(startApiKeyInput)
                                                keyEquivalent:@"k"
                                                      atIndex:1];
    apiKeyMenuItem.target = self;

    NSMenuItem* editFileMenuItem = [appMenu insertItemWithTitle:@"Edit Scene"
                                                         action:@selector(startFileEdit)
                                                  keyEquivalent:@"e"
                                                        atIndex:2];

    editFileMenuItem.target = self;

    NSMenuItem* sceneOpenMenuItem = [appMenu insertItemWithTitle:@"Open Scene..."
                                                           action:@selector(startFileOpen)
                                                    keyEquivalent:@"o"
                                                          atIndex:3];
    sceneOpenMenuItem.target = self;
}

+ (void)startApiKeyInput
{
    NSString* defaultsKeyString = [self apiKeyDefaultsName];
    NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
    NSString* defaultsValueString = [defaults stringForKey:defaultsKeyString];

    NSAlert* alert = [[NSAlert alloc] init];
    [alert setMessageText:@"Set API key default"];

    NSTextField *input = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 200, 24)];
    input.translatesAutoresizingMaskIntoConstraints = YES;
    input.editable = YES;
    input.selectable = YES;
    if (defaultsValueString != nil) {
        [input setStringValue:defaultsValueString];
        [input selectText:self];
    }

    [alert setAccessoryView:input];
    [alert addButtonWithTitle:@"Ok"];
    [alert addButtonWithTitle:@"Cancel"];
    NSInteger button = [alert runModal];
    if (button == NSAlertFirstButtonReturn) {
        [defaults setValue:[input stringValue] forKey:defaultsKeyString];
        GlfwApp::mapzenApiKey = std::string([[input stringValue] UTF8String]);
        GlfwApp::loadSceneFile();
    }
}

+ (void)startFileOpen
{
    NSOpenPanel* openPanel = [NSOpenPanel openPanel];
    openPanel.canChooseFiles = YES;
    openPanel.canChooseDirectories = NO;
    openPanel.allowsMultipleSelection = NO;

    NSInteger button = [openPanel runModal];
    if (button == NSFileHandlingPanelOKButton) {
        NSURL* url = [openPanel URLs].firstObject;
        LOG("Got file URL: %s", [[url absoluteString] UTF8String]);
        GlfwApp::sceneFile = std::string([[url absoluteString] UTF8String]);
        GlfwApp::sceneYaml.clear();
        GlfwApp::loadSceneFile();
    }
}

+ (void)startFileEdit
{
    NSString* file = [NSString stringWithUTF8String:GlfwApp::sceneFile.c_str()];
    NSURL* url = [NSURL fileURLWithPath:file];
    [[NSWorkspace sharedWorkspace] openURL:url];
}

+ (NSString*)apiKeyDefaultsName
{
    return @"mapzenApiKey";
}
@end

int main(int argc, char* argv[]) {

    auto platform = std::make_shared<OSXPlatform>();

    // Create the windowed app.
    GlfwApp::create(platform, 1024, 768);

    // Menu and window changes must come after window is created.
    [TGPreferences setup];

    GlfwApp::parseArgs(argc, argv);

    // Give it a chance to shutdown cleanly on CTRL-C
    signal(SIGINT, &GlfwApp::stop);

    // Loop until the user closes the window
    GlfwApp::run();

    // Clean up.
    GlfwApp::destroy();

}
