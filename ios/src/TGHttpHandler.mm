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

- (id)init
{
    self = [super init];

    if (self) {
        NSURLSessionConfiguration* defaultConfigObject = [NSURLSessionConfiguration defaultSessionConfiguration];
        NSString* cachePath = @"/tile_cache";
        NSURLCache* tileCache = [[NSURLCache alloc] initWithMemoryCapacity: 4 * 1024 * 1024 diskCapacity: 30 * 1024 * 1024 diskPath: cachePath];

        defaultConfigObject.URLCache = tileCache;
        defaultConfigObject.requestCachePolicy = NSURLRequestUseProtocolCachePolicy;
        defaultConfigObject.timeoutIntervalForRequest = 30;
        defaultConfigObject.timeoutIntervalForResource = 60;

        self.session = [NSURLSession sessionWithConfiguration: defaultConfigObject];
    }

    return self;
}

@end
