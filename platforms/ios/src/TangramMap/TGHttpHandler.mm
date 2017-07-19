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

@synthesize HTTPAdditionalHeaders = _HTTPAdditionalHeaders;

#pragma mark - Initializers

- (instancetype)init
{
    self = [super init];

    if (self) {
        [self initialSetupWithConfiguration:[TGHttpHandler defaultSessionConfiguration]];
    }

    return self;
}

- (instancetype)initWithCachePath:(NSString*)cachePath cacheMemoryCapacity:(NSUInteger)memoryCapacity cacheDiskCapacity:(NSUInteger)diskCapacity
{
    self = [super init];

    if (self) {
        [self initialSetupWithConfiguration:[TGHttpHandler defaultSessionConfiguration]];
        [self setCachePath:cachePath cacheMemoryCapacity:memoryCapacity cacheDiskCapacity:diskCapacity];
    }

    return self;
}

- (void)initialSetupWithConfiguration:(NSURLSessionConfiguration *)configuration {
    self.configuration = configuration;
    self.session = [NSURLSession sessionWithConfiguration:configuration];
    self.HTTPAdditionalHeaders = [[NSMutableDictionary alloc] init];

}

#pragma mark - Class Methods

+ (NSURLSessionConfiguration*)defaultSessionConfiguration
{
    NSURLSessionConfiguration* sessionConfiguration = [NSURLSessionConfiguration defaultSessionConfiguration];

    sessionConfiguration.timeoutIntervalForRequest = 30;
    sessionConfiguration.timeoutIntervalForResource = 60;

    return sessionConfiguration;
}

#pragma mark - Custom Getter/Setters

- (NSMutableDictionary *)HTTPAdditionalHeaders {
    return _HTTPAdditionalHeaders;
}

- (void)setAdditionalHTTPHeaders:(NSMutableDictionary *)HTTPAdditionalHeaders {
    _HTTPAdditionalHeaders = HTTPAdditionalHeaders;
    self.configuration.HTTPAdditionalHeaders = HTTPAdditionalHeaders;
    //Probably unnecessary, but for safety sake
    self.session.configuration.HTTPAdditionalHeaders = HTTPAdditionalHeaders;
}

- (void)setCachePath:(NSString*)cachePath cacheMemoryCapacity:(NSUInteger)memoryCapacity cacheDiskCapacity:(NSUInteger)diskCapacity
{
    NSURLCache* tileCache = [[NSURLCache alloc] initWithMemoryCapacity:memoryCapacity
                                                          diskCapacity:diskCapacity
                                                              diskPath:cachePath];

    self.configuration.URLCache = tileCache;
    self.configuration.requestCachePolicy = NSURLRequestUseProtocolCachePolicy;
}

#pragma mark - Instance Methods

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
