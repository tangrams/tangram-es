//
//  TGSceneUpdate.h
//  tangram
//
//  Created by Karim Naaji on 11/05/16.
//
//

#import <Foundation/Foundation.h>

@interface TGSceneUpdate : NSObject

- (instancetype)initWithPath:(nullable NSString *)path value:(nullable NSString *)value;

@property (copy, nonatomic) NSString* path;
@property (copy, nonatomic) NSString* value;

@end
