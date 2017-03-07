//
//  TGSceneUpdateStatus.h
//  TangramMap
//
//  Created by Karim Naaji on 3/07/17.
//  Copyright (c) 2017 Mapzen. All rights reserved.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@class TGSceneUpdate;

typedef NS_ENUM(NSInteger, TGSceneUpdateError) {
    TGSceneUpdateErrorPathNotFound,
    TGSceneUpdateErrorPathYAMLSyntaxError,
    TGSceneUpdateErrorValueYamlSyntaxError,
};

/**
 */
@interface TGSceneUpdateStatus : NSObject

///
@property (readonly, nonatomic) TGSceneUpdate* sceneUpdate;

///
@property (readonly, nonatomic) TGSceneUpdateError error;

NS_ASSUME_NONNULL_END

@end
