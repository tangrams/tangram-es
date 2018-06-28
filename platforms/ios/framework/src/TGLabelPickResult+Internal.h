//
//  TGLabelPickResult+Internal.h
//  TangramMap
//
//  Created by Karim Naaji on 2/27/17.
//  Copyright (c) 2017 Mapzen. All rights reserved.
//

#import "TGLabelPickResult.h"

@interface TGLabelPickResult ()

NS_ASSUME_NONNULL_BEGIN

/**
 Initializes a `TGLabelPickResult`.

 @param coordinates the geographic coordinates of the label pick result
 @param type the type of the label (text or icon)
 @param properties the set of properties associated to this label, keyed by their name

 @return an initialized `TGLabelPickResult`

 @note You shouldn't have to create a `TGLabelPickResult` yourself, those are returned as a result to
 a selection query on the `TGMapViewController` and initialized by the latter.
 */
- (instancetype) initWithCoordinates:(TGGeoPoint)coordinates type:(TGLabelType)type properties:(TGFeatureProperties *)properties;

NS_ASSUME_NONNULL_END

@end

