//
//  TGHttpHandler.h
//  tangram
//
//  Created by Karim Naaji on 11/23/16.
//
//

#import <Foundation/Foundation.h>

typedef void(^DownloadCompletionHandler)(NSData*, NSURLResponse*, NSError*);

@interface TGHttpHandler : NSObject

- (id)init;

- (id)initWithCachePath:(NSString*)cachePath cacheMemoryCapacity:(NSUInteger)memoryCapacity cacheDiskCapacity:(NSUInteger)diskCapacity;

- (void)downloadRequestAsync:(NSString*)url completionHandler:(DownloadCompletionHandler)completionHandler;

- (void)cancelDownloadRequestAsync:(NSString*)url;

- (void)setCachePath:(NSString*)cachePath cacheMemoryCapacity:(NSUInteger)memoryCapacity cacheDiskCapacity:(NSUInteger)diskCapacity;

@end
