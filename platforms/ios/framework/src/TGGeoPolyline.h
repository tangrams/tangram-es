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
 A shape made of connected line segments.

 The shape is defined by an ordered list of geographic coordinates.
 */
TG_EXPORT
@interface TGGeoPolyline : NSObject

/**
 Initializes a polyline with the specified coordinates.

 @param coordinates The array of coordinates defining the polyline. This data is copied into the new object.
 @return An initialized polyline.
 */
- (instancetype)initWithCoordinates:(const CLLocationCoordinate2D *)coordinates count:(NSUInteger)count;

/// The array of coordinates defining this polyline.
@property(readonly) CLLocationCoordinate2D *coordinates;

/// The number of coordinates in the polyline.
@property(readonly, nonatomic) NSUInteger count;

@end

NS_ASSUME_NONNULL_END
