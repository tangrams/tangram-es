//
//  TGMapData.h
//  TangramMap
//
//  Created by Karim Naaji on 2/24/16.
//  Copyright (c) 2017 Mapzen. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "TGGeoPoint.h"
#import "TGGeoPolygon.h"
#import "TGGeoPolyline.h"

/*
 Dictionary of feature properties keyed by their property name
 */
typedef NSDictionary<NSString *, NSString *> FeatureProperties;

/*
 A TGMapData is a convenience class to display point, polygons or polylines from a custom data source.
 The data source will be styled according to the scene file using the provided data source name.
 */
@interface TGMapData : NSObject

NS_ASSUME_NONNULL_BEGIN

/*
 Adds a point coordinate to the data source.

 @param coordinates the geographic coordinates of the point to add to the data source
 @param properties the feature properties
 */
- (void)addPoint:(TGGeoPoint)coordinates withProperties:(FeatureProperties *)properties;

/*
 Adds a polygon geometry to the data source.

 @param polygon the polygon geometry to add to the data source
 @param properties the feature properties
 */
- (void)addPolygon:(TGGeoPolygon *)polygon withProperties:(FeatureProperties *)properties;

/*
 Adds a polyline to the data source.

 @param polyline the polyline geometry to add to the data source
 @param properties the feature properties
 */
- (void)addPolyline:(TGGeoPolyline *)polyline withProperties:(FeatureProperties *)properties;

/*
 Adds a GeoJSON string to the data source. The string must be formatted according to the
 <a href="http://geojson.org/geojson-spec.html">GeoJSON specifications</a>.

 @param data the GeoJSON formatted string to add to the data source
 */
- (void)addGeoJson:(NSString *)data;

/*
 Clears the data source from all the added features.
 */
- (void)clear;

/*
 Definitely removes the data source from the map view.
 */
- (void)remove;

/// The name of the data source
@property (readonly, nonatomic) NSString* name;

NS_ASSUME_NONNULL_END

/// The map view this data source is on
@property (readonly, nonatomic) TGMapViewController* map;

@end
