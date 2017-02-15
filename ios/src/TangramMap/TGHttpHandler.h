//
//  TGHttpHandler.h
//  TangramMap
//
//  Created by Karim Naaji on 11/23/16.
//  Copyright (c) 2017 Mapzen. All rights reserved.
//

#import <Foundation/Foundation.h>

/**
 A network request completion callback, called when a download request of `TGHttpHandler`
 completed an asynchronous request.
*/
typedef void(^TGDownloadCompletionHandler)(NSData * _Nullable data, NSURLResponse * _Nullable response, NSError * _Nullable error);

NS_ASSUME_NONNULL_BEGIN

/**
 A configurable http handler used in a `TGMapViewController`.

 `TGMapViewController` provides a default http handler with the following configuration:

    - cache location: `/tangram_cache`
    - cache memory capacity: 4Mb
    - cache disk capacity: 30Mb

 To change this configuration, create a new http handler and set it to the map view with
 `-[TGMapViewController httpHandler]`.
 */
@interface TGHttpHandler : NSObject

/**
 Initializes a http handler with the default configuration.

 @return an initialized http handler
 */
- (instancetype)init;

/**
 Initializes a http handler with a user defined configuration.

 @param cachePath the location of the path in the client application bundle
 @param memoryCapacity the memory capacity of the cache, in bytes
 @param diskCapacity the disk capacity of cache, in bytes
 @return an initalized http handler with the provided configuration
 */
- (instancetype)initWithCachePath:(NSString *)cachePath cacheMemoryCapacity:(NSUInteger)memoryCapacity cacheDiskCapacity:(NSUInteger)diskCapacity;

/**
 Creates an asynchronous download request.

 @param url the URL of the download request
 @param completionHandler a handler to be called once the network request completed

 @note This method will be automatically called by the map view instance.
 */
- (void)downloadRequestAsync:(NSString *)url completionHandler:(TGDownloadCompletionHandler)completionHandler;

/**
 Cancels a download request for a specific URL.

 @param url the URL to cancel the network request for
 */
- (void)cancelDownloadRequestAsync:(NSString *)url;

/**
 Updates the http handler cache configuration.

 @param cachePath the location of the path in the client application bundle
 @param memoryCapacity the memory capacity of the cache, in bytes
 @param diskCapacity the disk capacity of cache, in bytes
 */
- (void)setCachePath:(NSString *)cachePath cacheMemoryCapacity:(NSUInteger)memoryCapacity cacheDiskCapacity:(NSUInteger)diskCapacity;

NS_ASSUME_NONNULL_END

@end
