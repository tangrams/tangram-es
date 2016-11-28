//
//  TGHttpHandler.h
//  tangram
//
//  Created by Karim Naaji on 11/23/16.
//
//

#import <Foundation/Foundation.h>

@interface TGHttpHandler : NSObject

- (id)initWithCachePath:(NSString*)cachePath cacheMemoryCapacity:(NSUInteger)memoryCapacity cacheDiskCapacity:(NSUInteger)diskCapacity;

@property (readonly, nonatomic) NSURLSession* session;

@end
