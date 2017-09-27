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
+ (void)startFileReload;
+ (void)copyEditorTextToClipboard;
+ (void)pasteEditorTextFromClipboard;
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

    NSMenuItem* editFileMenuItem = [appMenu insertItemWithTitle:@"Open Scene with Exernal Editor"
                                                         action:@selector(startFileEdit)
                                                  keyEquivalent:@"e"
                                                        atIndex:2];

    editFileMenuItem.target = self;

    NSMenuItem* sceneOpenMenuItem = [appMenu insertItemWithTitle:@"Open Scene..."
                                                           action:@selector(startFileOpen)
                                                    keyEquivalent:@"o"
                                                          atIndex:3];
    sceneOpenMenuItem.target = self;

    NSMenuItem* sceneReloadMenuItem = [appMenu insertItemWithTitle:@"Reload Scene"
                                                             action:@selector(startFileReload)
                                                      keyEquivalent:@"r"
                                                            atIndex:4];
    sceneReloadMenuItem.target = self;

    NSMenuItem* copyMenuItem = [appMenu insertItemWithTitle:@"Copy"
                                                     action:@selector(copyEditorTextToClipboard)
                                              keyEquivalent:@"c"
                                                    atIndex:5];
    copyMenuItem.target = self;

    NSMenuItem* pasteMenuItem = [appMenu insertItemWithTitle:@"Paste"
                                                      action:@selector(pasteEditorTextFromClipboard)
                                               keyEquivalent:@"v"
                                                     atIndex:6];
    pasteMenuItem.target = self;
}

+ (void)startApiKeyInput
{
    NSString* defaultsKeyString = [self apiKeyDefaultsName];
    NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
    NSString* defaultsValueString = [defaults stringForKey:defaultsKeyString];

    NSTextField *input = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 200, 24)];
    input.translatesAutoresizingMaskIntoConstraints = YES;
    input.editable = YES;
    input.selectable = YES;
    if (defaultsValueString != nil) {
        [input setStringValue:defaultsValueString];
    }

    NSAlert* alert = [[NSAlert alloc] init];
    [alert setMessageText:@"Set API key default"];
    [alert setAccessoryView: input];
    [alert addButtonWithTitle:@"Ok"];
    [alert addButtonWithTitle:@"Cancel"];
    [alert layout];
    [alert.window makeFirstResponder:alert.accessoryView];

    [alert beginSheetModalForWindow:[[NSApplication sharedApplication] mainWindow] completionHandler:^(NSModalResponse returnCode) {
        if (returnCode == NSAlertFirstButtonReturn) {
            [defaults setValue:[input stringValue] forKey:defaultsKeyString];
            GlfwApp::mapzenApiKey = std::string([[input stringValue] UTF8String]);
            GlfwApp::loadSceneFile();
        }
    }];
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
        LOG("Got URL to open: %s", [[url absoluteString] UTF8String]);
        // TODO: When generic URL support is added to scene loading, we should
        // use the full URL here instead of just the path.
        GlfwApp::sceneFile = std::string([[url path] UTF8String]);
        GlfwApp::sceneYaml.clear();
        GlfwApp::loadSceneFile();
    }
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

+ (void)copyEditorTextToClipboard
{
    NSText* fieldEditor = [[[NSApplication sharedApplication] keyWindow] fieldEditor:NO forObject:nil];
    if (fieldEditor) {
        NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
        [pasteboard clearContents];
        [pasteboard setString:[fieldEditor string] forType:NSPasteboardTypeString];
    }
}

+ (void)pasteEditorTextFromClipboard
{
    NSText* fieldEditor = [[[NSApplication sharedApplication] keyWindow] fieldEditor:NO forObject:nil];
    if (fieldEditor) {
        NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
        [fieldEditor setString:[pasteboard stringForType:NSPasteboardTypeString]];
    }
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
