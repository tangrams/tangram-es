#ifdef PLATFORM_OSX

#include <CoreFoundation/CoreFoundation.h>

#include "platform.h"

void logMsg(const char* fmt, ...) {

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

}

int readInternalFile(const char* path, void*& buff, long& length) {

    // Resolve complete file path
    CFBundleRef mainBundle = CFBundleGetMainBundle();
    CFStringRef resourceName = CFStringCreateWithCString(nullptr, path, kCFStringEncodingUTF8);
    CFURLRef fileURL = CFBundleCopyResourceURL(mainBundle, resourceName, nullptr, nullptr);

    // Open file
    FILE* file = fopen(CFStringGetCStringPtr(CFURLCopyPath(fileURL), kCFStringEncodingUTF8), "r");
    
    if (file == nullptr) {
        // Failed to open file!
        logMsg("Failed to open file at path: %s\n", path);
        return -1;
    }
    
    // Get length in bytes
    fseek(file, 0, SEEK_END);
    length = ftell(file);

    // Allocate memory
    buff = malloc(length);

    // Read file into memory
    fseek(file, 0, SEEK_SET);
    fread(buff, sizeof(char), length, file);
    
    // Close file & clean up
    fclose(file);
    CFRelease(resourceName);
    CFRelease(fileURL);

    // Return no error on completion
    return 0;
}

#endif //PLATFORM_OSX
