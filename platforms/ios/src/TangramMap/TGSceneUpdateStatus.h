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

/// Description of error status triggered by a scene update
typedef NS_ENUM(NSInteger, TGSceneUpdateError) {
    /// The path of the scene update was not found on the scene file
    TGSceneUpdateErrorPathNotFound,
    /// The YAML syntax of the scene udpate path has a syntax error
    TGSceneUpdateErrorPathYAMLSyntaxError,
    /// The YAML syntax of the scene update value has a syntax error
    TGSceneUpdateErrorValueYamlSyntaxError,
};

/**
 Holds information relative to a scene update error status
 */
@interface TGSceneUpdateStatus : NSObject

/// A scene update holding the path and the scene update value
@property (readonly, nonatomic) TGSceneUpdate* sceneUpdate;

/// The type of error the scene update triggered when applied to the scene
@property (readonly, nonatomic) TGSceneUpdateError error;

NS_ASSUME_NONNULL_END

@end
