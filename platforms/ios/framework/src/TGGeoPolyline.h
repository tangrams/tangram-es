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

NS_ASSUME_NONNULL_BEGIN

/**
 Helper class to contain a polyline geometry for use in `-[TGMarker polyline]`.

 Set the geometry of a marker to a polyline along the given coordinates; _coordinates is a
 pointer to a sequence of _count LngLats; markers can have their geometry set multiple times
 with possibly different geometry types; returns true if the marker ID was found and
 successfully updated, otherwise returns false.
 */
TG_EXPORT
@interface TGGeoPolyline : NSObject

/**
 Inits a `TGGeoPolyline` and allocate enough memory to hold `size` geographic coordinates.
 */
- (instancetype)initWithCoordinates:(const CLLocationCoordinate2D *)coordinates count:(NSUInteger)count;

/**
 Gets a pointer to the geographic coordinates describing this poyline of array length `-[TGGeoPolyline count:]`.

 @return a pointer to the list of geographic coordinates describing this polyline
 */
@property(readonly) CLLocationCoordinate2D *coordinates;

/**
 Gets the number of geographic coordinates describing this polyline.

 @return the number of geographic coordinates in this polyline
 */
@property(readonly, nonatomic) NSUInteger count;

@end

NS_ASSUME_NONNULL_END
