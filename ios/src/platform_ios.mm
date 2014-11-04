#ifdef PLATFORM_IOS

#import <Foundation/Foundation.h>

#include "platform.h"

void logMsg(const char* fmt, ...) {

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

}

int readInternalFile(const char* path, void*& buff, long& length) {
    
    // Resolve full file path
    NSString* resourcePath = [[NSBundle mainBundle] resourcePath];
    NSString* internalPath = [NSString stringWithUTF8String:path];
    NSString* fullPath = [[resourcePath stringByAppendingString:@"/"] stringByAppendingString:internalPath];
    
    // Open file for reading
    FILE* file = fopen([fullPath UTF8String], "r");
    
    if (file == nullptr) {
        logMsg("Failed to open file with path: %s\nResolved to full path: %s\n", path, [fullPath UTF8String]);
        return -1;
    }
    
    // Allocate memory for data
    fseek(file, 0, SEEK_END);
    length = ftell(file);
    buff = malloc(length);
    
    // Read data
    fseek(file, 0, SEEK_SET);
    fread(buff, sizeof(char), length, file);
    
    // Clean up
    fclose(file);
    
    return 0;
}

#endif
