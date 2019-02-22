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
#import "TGGeoPoints.h"

NS_ASSUME_NONNULL_BEGIN

/**
 Helper class to contain a polygon geometry for use in `-[TGMarker polygon]`.

 The polygon winding order and internal polygons must be set according to the <a href="http://geojson.org/geojson-spec.html#polygon">
 GeoJSON specification</a>.
 */
TG_EXPORT
@interface TGGeoPolygon : TGGeoPoints

/**
 Inits a polygon and allocates enough memory to hold `size` geographic coordinates in the polygon paths.
 */
- (instancetype)initWithCoordinates:(CLLocationCoordinate2D *)coordinates count:(NSUInteger)count;


- (instancetype)initWithCoordinates:(CLLocationCoordinate2D *)coordinates count:(NSUInteger)count interiorPolygons:(NSArray<TGGeoPolygon *> *)interiorPolygons;

@property(readonly) NSArray<TGGeoPolygon *> *interiorPolygons;

@end

NS_ASSUME_NONNULL_END
