#ifdef PLATFORM_IOS

#import <Foundation/Foundation.h>
#import <utility>
#import <cstdio>
#import <cstdarg>

#include "platform.h"

void logMsg(const char* fmt, ...) {

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

}

NSString* resolveResourcePath(const char* _path) {
    
    NSString* resourcePath = [[[NSBundle mainBundle] resourcePath] stringByAppendingString:@"/"];
    NSString* internalPath = [NSString stringWithUTF8String:_path];
    return [resourcePath stringByAppendingString:internalPath];
    
}

std::string stringFromResource(const char* _path) {
    
    NSString* path = resolveResourcePath(_path);
    NSString* str = [NSString stringWithContentsOfFile:path
                                              encoding:NSASCIIStringEncoding
                                                 error:NULL];

    if (str == nil) {
        logMsg("Failed to read file at path: %s\n", _path);
        return std::move(std::string());
    }
    
    return std::move(std::string([str UTF8String]));
}

#endif //PLATFORM_IOS
