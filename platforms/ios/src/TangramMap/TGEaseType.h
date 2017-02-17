//
//  TGEaseType.h
//  TangramMap
//
//  Created by Karim Naaji on 2/21/17.
//  Copyright (c) 2017 Mapzen. All rights reserved.
//

/**
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
