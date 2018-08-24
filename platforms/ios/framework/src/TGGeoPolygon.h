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

NS_ASSUME_NONNULL_BEGIN

/**
 Helper class to contain a polygon geometry for use in `-[TGMarker polygon]`.

 The polygon winding order and internal polygons must be set according to the <a href="http://geojson.org/geojson-spec.html#polygon">
 GeoJSON specification</a>.
 */
TG_EXPORT
@interface TGGeoPolygon : NSObject

/**
 Inits a polygon and allocates enough memory to hold `size` geographic coordinates in the polygon paths.
 */
- (instancetype)initWithSize:(NSUInteger)size;

/**
 Starts a new polygon path for a given coordinate.

 @param point the coordinate to start a polygon path
 */
- (void)startPath:(CLLocationCoordinate2D)point;

/**
 Starts a new polygon path for a given coordinate and allocate enough memory to hold `size` coordinates
 in the polygon geometry.

 @param point the coordinate to start a polygon path
 @param size the size of the polygon path
 */
- (void)startPath:(CLLocationCoordinate2D)point withSize:(NSUInteger)size;

/**
 Adds a single coordinate to the current polygon path.

 @param point the coordinate to add to the current path

 @note This operation is a no-op if no polygon path were already started.
 */
- (void)addPoint:(CLLocationCoordinate2D)point;

/**
 Returns a pointer to a sequence of coordinates with a total array length equal to `-[TGGeoPolygon count:]`.

 @return a pointer to the list of polygons coordinates
 */
- (CLLocationCoordinate2D *)coordinates;

/**
 The list of polygon `rings` contained in this geometry.
 The sum of the values in `-[TGGeoPolygon rings:]` sum up to `-[TGeoPolygon count:]`.

 @return a pointer to the list of polygon rings.
 */
- (int *)rings;

/**
 The polygon coordinates count.

 @return the number of geographic coordinates of this polygon
 */
- (NSUInteger)count;

/**
 The polygon rings count.

 @return the number of rings of this polygon
 */
- (NSUInteger)ringsCount;

/**
 Removes all coordinates and rings of this polygon.
 */
- (void)removeAll;

NS_ASSUME_NONNULL_END

@end
