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
- (instancetype)initWithSize:(NSUInteger)size;

/**
 Adds a geographic coordinate to the polyline.

 @param point the geographic coordinate to add to this polyline
 */
- (void)addPoint:(CLLocationCoordinate2D)point;

/**
 Gets the number of geographic coordinates describing this polyline.

 @return the number of geographic coordinates in this polyline
 */
- (NSUInteger)count;

/**
 Gets a pointer to the geographic coordinates describing this poyline of array length `-[TGGeoPolyline count:]`.

 @return a pointer to the list of geographic coordinates describing this polyline
 */
- (CLLocationCoordinate2D *)coordinates;

/**
 Removes all coordinates of this polyline.
 */
- (void)removeAll;

NS_ASSUME_NONNULL_END

@end
