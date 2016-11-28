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

- (id)initWithCachePath:(NSString*)cachePath cacheMemoryCapacity:(NSUInteger)memoryCapacity cacheDiskCapacity:(NSUInteger)diskCapacity;

- (void)downloadAsync:(NSString*)nsUrl completionHandler:(DownloadCompletionHandler)completionHandler;

@property (readonly, nonatomic) NSURLSession* session;


@end
