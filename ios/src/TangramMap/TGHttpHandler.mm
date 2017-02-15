//
//  TGHttpHandler.mm
//  TangramMap
//
//  Created by Karim Naaji on 11/23/16.
//  Copyright (c) 2017 Mapzen. All rights reserved.
//

#import "TGHttpHandler.h"

@interface TGHttpHandler()

@property (strong, nonatomic) NSURLSession* session;
@property (strong, nonatomic) NSURLSessionConfiguration* configuration;

+ (NSURLSessionConfiguration*)defaultConfiguration;

@end

@implementation TGHttpHandler

- (instancetype)init
{
    self = [super init];

    if (self) {
        self.configuration = [TGHttpHandler defaultSessionConfiguration];
        self.session = [NSURLSession sessionWithConfiguration:self.configuration];
    }

    return self;
}

- (instancetype)initWithCachePath:(NSString*)cachePath cacheMemoryCapacity:(NSUInteger)memoryCapacity cacheDiskCapacity:(NSUInteger)diskCapacity
{
    self = [super init];

    if (self) {
        self.configuration = [TGHttpHandler defaultSessionConfiguration];
        [self setCachePath:cachePath cacheMemoryCapacity:memoryCapacity cacheDiskCapacity:diskCapacity];
        self.session = [NSURLSession sessionWithConfiguration:self.configuration];
    }

    return self;
}

+ (NSURLSessionConfiguration*)defaultSessionConfiguration
{
    NSURLSessionConfiguration* sessionConfiguration = [NSURLSessionConfiguration defaultSessionConfiguration];

    sessionConfiguration.timeoutIntervalForRequest = 30;
    sessionConfiguration.timeoutIntervalForResource = 60;

    return sessionConfiguration;
}

- (void)setCachePath:(NSString*)cachePath cacheMemoryCapacity:(NSUInteger)memoryCapacity cacheDiskCapacity:(NSUInteger)diskCapacity
{
    NSURLCache* tileCache = [[NSURLCache alloc] initWithMemoryCapacity:memoryCapacity
                                                          diskCapacity:diskCapacity
                                                              diskPath:cachePath];

    self.configuration.URLCache = tileCache;
    self.configuration.requestCachePolicy = NSURLRequestUseProtocolCachePolicy;
}

- (void)downloadRequestAsync:(NSString*)url completionHandler:(TGDownloadCompletionHandler)completionHandler
{
    NSURLSessionDataTask* dataTask = [self.session dataTaskWithURL:[NSURL URLWithString:url]
                                                                      completionHandler:completionHandler];

    [dataTask resume];
}

- (void)cancelDownloadRequestAsync:(NSString*)url
{
    [self.session getTasksWithCompletionHandler:^(NSArray* dataTasks, NSArray* uploadTasks, NSArray* downloadTasks) {
        for (NSURLSessionTask* task in dataTasks) {
            if ([[task originalRequest].URL.absoluteString isEqualToString:url]) {
                [task cancel];
                break;
            }
        }
    }];
}

@end
