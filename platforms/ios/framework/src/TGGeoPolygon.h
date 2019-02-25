//
//  TGGeoPolygon.h
//  TangramMap
//
//  Created by Karim Naaji on 10/27/16.
//  Updated by Matt Blair on 8/21/18.
//  Copyright (c) 2017 Mapzen. All rights reserved.
//

#import <CoreLocation/CoreLocation.h>
#import <Foundation/Foundation.h>
#import "TGExport.h"
#import "TGGeoPolyline.h"

NS_ASSUME_NONNULL_BEGIN

/**
 A closed polygon with optional holes.

 The polygon is defined by a list of rings. Each ring is represented with a `TGGeoPolyline`. The first and last points
 in each ring are joined to form a closed shape. The first ring represents the exterior of the polygon and any
 subsequent rings become holes that are cut out of the polygon.
 */
TG_EXPORT
@interface TGGeoPolygon : NSObject

/**
 Initializes a polygon from the specified list of rings.

 @param rings The list of rings defining the polygon.
 @return An initialized polygon.
 */
- (instancetype)initWithRings:(NSArray<TGGeoPolyline *> *)rings;

/// The array of rings defining this polygon.
@property(readonly) NSArray<TGGeoPolyline *> *rings;

@end

NS_ASSUME_NONNULL_END
