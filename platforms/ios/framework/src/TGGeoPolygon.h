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
 Helper class to contain a polygon geometry for use in `-[TGMarker polygon]`.

 The polygon winding order and internal polygons must be set according to the <a href="http://geojson.org/geojson-spec.html#polygon">
 GeoJSON specification</a>.
 */
TG_EXPORT
@interface TGGeoPolygon : NSObject

/**
 Inits a polygon and allocates enough memory to hold `size` geographic coordinates in the polygon paths.
 */
- (instancetype)initWithRings:(NSArray<TGGeoPolyline *> *)rings;

@property(readonly) NSArray<TGGeoPolyline *> *rings;

@end

NS_ASSUME_NONNULL_END
