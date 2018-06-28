//
//  TGGeoPolygon.h
//  TangramMap
//
//  Created by Karim Naaji on 10/27/16.
//  Copyright (c) 2017 Mapzen. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "TGGeoPoint.h"

NS_ASSUME_NONNULL_BEGIN

/**
 Helper class to contain a polygon geometry for use in `-[TGMarker polygon]`.

 The polygon winding order and internal polygons must be set according to the <a href="http://geojson.org/geojson-spec.html#polygon">
 GeoJSON specification</a>.
 */
@interface TGGeoPolygon : NSObject

/**
 Inits a polygon and allocates enough memory to hold `size` geographic coordinates in the polygon paths.
 */
- (instancetype)initWithSize:(unsigned int)size;

/**
 Starts a new polygon path for a given `TGGeoPoint`.

 @param latlon the coordinate to start a polygon path
 */
- (void)startPath:(TGGeoPoint)latlon;

/**
 Starts a new polygon path for a given `TGGeoPoint` and allocate enough memory to hold `size` coordinates
 in the polygon geometry.

 @param latlon the coordinate to start a polygon path
 @param size the size of the polygon path
 */
- (void)startPath:(TGGeoPoint)latlon withSize:(unsigned int)size;

/**
 Adds a single coordinate to the current polygon path.

 @param latlon the coordinate to add to the current path

 @note This operation is a no-op if no polygon path were already started.
 */
- (void)addPoint:(TGGeoPoint)latlon;

/**
 Returns a pointer to a sequence of `TGGeoPoint` with a total array length equal to `-[TGGeoPolygon count:]`.

 @return a pointer to the list of polygons coordinates
 */
- (TGGeoPoint*)coordinates;

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
