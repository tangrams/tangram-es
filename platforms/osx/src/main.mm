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
        GlfwApp::apiKey = std::string([storedApiKey UTF8String]);
    }

    // Set up menu shortcuts.
    NSMenu *mainMenu = [[NSApplication sharedApplication] mainMenu];

    // Set up Tangram ES menu.
    NSMenu *appMenu = [[mainMenu itemAtIndex:0] submenu];
    [appMenu insertItemWithTitle:@"API Key..."
                          action:@selector(startApiKeyInput)
                   keyEquivalent:@"k"
                         atIndex:1].target = self;

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

    // Set up Edit menu.
    NSMenuItem* editMenuItem = [mainMenu insertItemWithTitle:@"" action:nil keyEquivalent:@"" atIndex:2];
    NSMenu* editMenu = [[NSMenu alloc] init];
    [editMenuItem setSubmenu:editMenu];
    [editMenu setTitle:@"Edit"];

    [editMenu addItemWithTitle:@"Copy"
                        action:@selector(copyEditorTextToClipboard)
                 keyEquivalent:@"c"].target = self;

    [editMenu addItemWithTitle:@"Paste"
                        action:@selector(pasteEditorTextFromClipboard)
                 keyEquivalent:@"v"].target = self;
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
            GlfwApp::apiKey = std::string([[input stringValue] UTF8String]);
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
        GlfwApp::sceneFile = [[url absoluteString] UTF8String];
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
    return @"apiKey";
}
@end

int main(int argc, char* argv[]) {

    auto platform = std::make_shared<OSXPlatform>();

    // Create the windowed app.
    GlfwApp::create(platform, 1024, 768);

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
