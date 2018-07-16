//
//  TGURLHandler.h
//  TangramMap
//
//  Created by Karim Naaji on 11/23/16.
//  Updated by Matt Blair on 7/16/18.
//  Copyright (c) 2017 Mapzen. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "TGExport.h"

/**
 A URL request completion callback, called when a download request of `TGURLHandler`
 completed an asynchronous request.
*/
typedef void(^TGDownloadCompletionHandler)(NSData * _Nullable data, NSURLResponse * _Nullable response, NSError * _Nullable error);

NS_ASSUME_NONNULL_BEGIN

/**
 A configurable URL handler used in a `TGMapViewController`.

 `TGMapView` provides a default URL handler with the following configuration:

    - cache location: `/tangram_cache`
    - cache memory capacity: 4Mb
    - cache disk capacity: 30Mb

 To change this configuration, create a new URL handler and set it to the map view with `-[TGMapView urlHandler]`.
 */
TG_EXPORT
@interface TGURLHandler : NSObject

/**
 Additional HTTP headers analogous to the standard NSURLSessionConfiguration property of the same name.
 */
@property(nonatomic, strong) NSMutableDictionary *HTTPAdditionalHeaders;

/**
 Initializes a URL handler with the default configuration.

 @return an initialized URL handler
 */
- (instancetype)init;

/**
 Initializes a URL handler with the custom configuration.

 @param configuration custom NSURLConfiguration object
 @return an initialized URL handler
 */
- (instancetype)initWithSessionConfiguration:(NSURLSessionConfiguration *)configuration;

/**
 Initializes a URL handler with a user defined configuration.

 @param cachePath the location of the path in the client application bundle
 @param memoryCapacity the memory capacity of the cache, in bytes
 @param diskCapacity the disk capacity of cache, in bytes
 @return an initalized URL handler with the provided configuration
 */
- (instancetype)initWithCachePath:(NSString *)cachePath
              cacheMemoryCapacity:(NSUInteger)memoryCapacity
                cacheDiskCapacity:(NSUInteger)diskCapacity;

/**
 Creates an asynchronous download request.

 @param url the URL of the download request
 @param completionHandler a handler to be called once the URL request completed
 @return an integer that uniquely identifies the resulting task within this handler

 @note This method will be automatically called by the map view instance.
 */
- (NSUInteger)downloadRequestAsync:(NSString *)url completionHandler:(TGDownloadCompletionHandler)completionHandler;

/**
 Cancels a download request for a specific URL.

 @param taskIdentifier the taskIdentifier to cancel the network request for
 */
- (void)cancelDownloadRequestAsync:(NSUInteger)taskIdentifier;

/**
 Updates the URL handler cache configuration.

 @param cachePath the location of the path in the client application bundle
 @param memoryCapacity the memory capacity of the cache, in bytes
 @param diskCapacity the disk capacity of cache, in bytes
 */
- (void)setCachePath:(NSString *)cachePath
 cacheMemoryCapacity:(NSUInteger)memoryCapacity
   cacheDiskCapacity:(NSUInteger)diskCapacity;

NS_ASSUME_NONNULL_END

@end
