//
//  TGGeoPolyline.h
//  TangramMap
//
//  Created by Karim Naaji on 10/27/16.
//  Updated by Matt Blair on 8/21/18.
//  Copyright (c) 2017 Mapzen. All rights reserved.
//

#import <CoreLocation/CoreLocation.h>
#import <Foundation/Foundation.h>
#import "TGExport.h"
#import "TGGeoPoints.h"

NS_ASSUME_NONNULL_BEGIN

/**
 Helper class to contain a polyline geometry for use in `-[TGMarker polyline]`.

 Set the geometry of a marker to a polyline along the given coordinates; _coordinates is a
 pointer to a sequence of _count LngLats; markers can have their geometry set multiple times
 with possibly different geometry types; returns true if the marker ID was found and
 successfully updated, otherwise returns false.
 */
TG_EXPORT
@interface TGGeoPolyline : TGGeoPoints

/**
 Inits a `TGGeoPolyline` and allocate enough memory to hold `size` geographic coordinates.
 */
- (instancetype)initWithCoordinates:(const CLLocationCoordinate2D *)coordinates count:(NSUInteger)count;

@end

NS_ASSUME_NONNULL_END
