//
//  TGGeoPolyline.h
//  TangramMap
//
//  Created by Karim Naaji on 10/27/16.
//  Copyright (c) 2017 Mapzen. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "TGGeoPoint.h"

NS_ASSUME_NONNULL_BEGIN

/**
 Helper class to contain a polyline geometry for use in `-[TGMarker polyline]`.

 Set the geometry of a marker to a polyline along the given coordinates; _coordinates is a
 pointer to a sequence of _count LngLats; markers can have their geometry set multiple times
 with possibly different geometry types; returns true if the marker ID was found and
 successfully updated, otherwise returns false.
 */
@interface TGGeoPolyline : NSObject

/**
 Inits a `TGGeoPolyline` and allocate enough memory to hold `size` geographic coordinates.
 */
- (instancetype)initWithSize:(unsigned int)size;

/**
 Adds a geographic coordinate to the polyline.

 @param latlon the geographic coordinate to add to this polyline
 */
- (void)addPoint:(TGGeoPoint)latlon;

/**
 Gets the number of geographic coordinates describing this polyline.

 @return the number of geographic coordinates in this polyline
 */
- (NSUInteger)count;

/**
 Gets a pointer to the geographic coordinates describing this poyline of array length `-[TGGeoPolyline count:]`.

 @return a pointer to the list of geographic coordinates describing this polyline
 */
- (TGGeoPoint*)coordinates;

/**
 Removes all coordinates of this polyline.
 */
- (void)removeAll;

NS_ASSUME_NONNULL_END

@end
