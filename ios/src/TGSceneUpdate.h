//
//  TGSceneUpdate.h
//  tangram
//
//  Created by Karim Naaji on 11/05/16.
//
//

#import <Foundation/Foundation.h>

@interface TGSceneUpdate : NSObject

- (id)initWithPath:(NSString *)path value:(NSString *)value;

@property (strong, nonatomic) NSString* path;
@property (strong, nonatomic) NSString* value;

@end
