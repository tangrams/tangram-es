//
//  TGHttpHandler.mm
//  tangram
//
//  Created by Karim Naaji on 11/23/16.
//
//

#import "TGHttpHandler.h"

@interface TGHttpHandler () {
}

@property (strong, nonatomic) NSURLSession* session;

@end

@implementation TGHttpHandler

- (id)initWithCachePath:(NSString*)cachePath cacheMemoryCapacity:(NSUInteger)memoryCapacity cacheDiskCapacity:(NSUInteger)diskCapacity
{
    self = [super init];

    if (self) {
        NSURLSessionConfiguration* defaultConfigObject = [NSURLSessionConfiguration defaultSessionConfiguration];
        NSURLCache* tileCache = [[NSURLCache alloc] initWithMemoryCapacity:memoryCapacity
                                                              diskCapacity:diskCapacity
                                                                  diskPath:cachePath];

        defaultConfigObject.URLCache = tileCache;
        defaultConfigObject.requestCachePolicy = NSURLRequestUseProtocolCachePolicy;
        defaultConfigObject.timeoutIntervalForRequest = 30;
        defaultConfigObject.timeoutIntervalForResource = 60;

        self.session = [NSURLSession sessionWithConfiguration: defaultConfigObject];
    }

    return self;
}

@end
