//
//  TGHttpHandler.h
//  tangram
//
//  Created by Karim Naaji on 11/23/16.
//
//

#import <Foundation/Foundation.h>

@interface TGHttpHandler : NSObject

- (id)init;

@property (readonly, nonatomic) NSURLSession* session;

@end
