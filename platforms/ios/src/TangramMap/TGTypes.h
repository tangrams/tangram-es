//
//  TGTypes.h
//  TangramMap
//
//  Created by Karim Naaji on 4/20/17.
//  Copyright (c) 2017 Mapzen. All rights reserved.
//

#import <Foundation/Foundation.h>

/// Indicates an error occurred in the Tangram Framework.
extern NSString* const TGErrorDomain;

/*
 These enumerations describe an ease type function to be used for camera or other transition animation.
 The function is being used to interpolate between the start and end position of the animation.
 */
typedef NS_ENUM(NSInteger, TGEaseType) {
    /// Linear ease type `f(t) = t`
    TGEaseTypeLinear = 0,
    /// Cube ease type `f(t) = (-2 * t + 3) * t * t`
    TGEaseTypeCubic,
    /// Quint ease type `f(t) = (6 * t * t - 15 * t + 10) * t * t * t`
    TGEaseTypeQuint,
    /// Sine ease type `f(t) = 0.5 - 0.5 * cos(PI * t)`
    TGEaseTypeSine,
};

/**
 Description for a <a href="https://mapzen.com/documentation/tangram/Cameras-Overview/#camera-types">
 Tangram camera type</a>.
 */
typedef NS_ENUM(NSInteger, TGCameraType) {
    /// Type for a <a href="https://mapzen.com/documentation/tangram/Cameras-Overview/#perspective-camera">perspective camera</a>
    TGCameraTypePerspective = 0,
    /// Type for an <a href="https://mapzen.com/documentation/tangram/Cameras-Overview/#isometric-camera">isometric camera</a>
    TGCameraTypeIsometric,
    /// Type for a <a href="https://mapzen.com/documentation/tangram/Cameras-Overview/#flat-camera">flat camera</a>
    TGCameraTypeFlat,
};

/// Description of error status from Tangram
typedef NS_ENUM(NSInteger, TGError) {
    /// The path of the scene update was not found on the scene file
    TGErrorSceneUpdatePathNotFound,
    /// The YAML syntax of the scene udpate path has a syntax error
    TGErrorSceneUpdatePathYAMLSyntaxError,
    /// The YAML syntax of the scene update value has a syntax error
    TGErrorSceneUpdateValueYAMLSyntaxError,
    /// An error code for markers
    TGErrorMarker,
};

/// Debug flags to render additional information about various map components
typedef NS_ENUM(NSInteger, TGDebugFlag) {
    /// While on, the set of tiles currently being drawn will not update to match the view
    TGDebugFlagFreezeTiles = 0,
    /// Apply a color change to every other zoom level to visualize proxy tile behavior
    TGDebugFlagProxyColors,
    /// Draw tile boundaries
    TGDebugFlagTileBounds,
    /// Draw tile infos (tile coordinates)
    TGDebugFlagTileInfos,
    /// Draw label bounding boxes and collision grid
    TGDebugFlagLabels,
    /// Draw tangram infos (framerate, debug log...)
    TGDebugFlagTangramInfos,
    /// Draw all labels (including labels being occluded)
    TGDebugFlagDrawAllLabels,
    /// Draw tangram frame graph stats
    TGDebugFlagTangramStats,
    /// Draw feature selection framebuffer
    TGDebugFlagSelectionBuffer,
};

