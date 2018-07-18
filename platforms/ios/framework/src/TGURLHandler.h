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
 A URL request completion callback, called when a download request of `TGURLHandler` completes an asynchronous request.
*/
typedef void(^TGDownloadCompletionHandler)(NSData * _Nullable data, NSURLResponse * _Nullable response, NSError * _Nullable error);

NS_ASSUME_NONNULL_BEGIN

/**
 A URL Handler interface for creating cancellable asynchronous URL requests.
 */
@protocol TGURLHandler <NSObject>

/**
 Create an asynchronous download request.

 @param url The URL to download.
 @param completionHandler A handler to be called once the URL request completed.
 @return A task identifier that uniquely identifies the URL request within this handler.

 @note This method will be automatically called by the map view instance.
 */
- (NSUInteger)downloadRequestAsync:(NSURL *)url completionHandler:(TGDownloadCompletionHandler)completionHandler;

/**
 Cancel a previous download request.

 @param taskIdentifier The task identifier for the request to cancel.
 */
- (void)cancelDownloadRequestAsync:(NSUInteger)taskIdentifier;

@end // protocol TGURLHandler

/**
 A default implementation of the `TGURLHandler` interface.
 */
TG_EXPORT
@interface TGDefaultURLHandler : NSObject<TGURLHandler>

/**
 Initialize a URL handler with the default configuration.

 @return an initialized URL handler
 */
- (instancetype)init;

/**
 Initialize a URL handler with a custom configuration.

 @param configuration custom NSURLConfiguration object
 @return an initialized URL handler
 */
- (instancetype)initWithConfiguration:(NSURLSessionConfiguration *)configuration;

/**
 Get the default configuration object for this URL handler type.

 This configuration has a modified request timeout and a URL cache in memory and on disk. You can use this object as a
 base for further configuration modifications.
 */
+ (NSURLSessionConfiguration*)defaultConfiguration;

@end // interface TGDefaultURLHandler

NS_ASSUME_NONNULL_END

