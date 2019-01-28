//
//  TGMapData.h
//  TangramMap
//
//  Created by Karim Naaji on 2/24/16.
//  Updated by Matt Blair on 8/21/18.
//  Copyright (c) 2017 Mapzen. All rights reserved.
//

#import <CoreLocation/CoreLocation.h>
#import <Foundation/Foundation.h>
#import "TGExport.h"


/**
 Dictionary of feature properties keyed by their property name
 */
typedef NSDictionary<NSString *, NSString *> TGFeatureProperties;

@class TGMapView;
@class TGGeoPolygon;
@class TGGeoPolyline;

/**
 A `TGMapData` is a convenience class to display point, polygons or polylines from a dynamic data layer.
 The data layer will be styled according to the scene file using the provided data layer name.

 In your stylesheet, add a layer with the name of the data layer you want to add in your client application:

 ```
 layers:
     mz_route_line_transit:
        data: { source: mz_route_line_transit }
     draw:
        polylines:
            color: function() { return feature.color || '#06a6d4'; }
            order: 500
            width: 10px
 ```

 In your implementation, to add a polyline fitting under the `mz_route_line_transit` layer:

 ```swift
 // Create a data layer in the TGMapView mapView
 var dataLayer = mapView.addDataLayer(name: "mz_Route_line_transit");

 var line = TGGeoPolyline()

 // Add some coordinates to the polyline
 line.addPoint(point: CLLocationCoordinate2DMake(longitude0, latitude0))
 line.addPoint(point: CLLocationCoordinate2DMake(longitude1, latitude1))

 // Set the data properties
 var properties = ["type": "line", "color": "#D2655F"]

 // Add the line to the data layer
 dataLayer.add(polyline: line, properties: properties);
 ```
 */
TG_EXPORT
@interface TGMapData : NSObject

NS_ASSUME_NONNULL_BEGIN

/**
 Adds a point coordinate to the data source.

 @param point the geographic coordinates of the point to add to the data source
 @param properties the feature properties
 */
- (void)addPoint:(CLLocationCoordinate2D)point withProperties:(TGFeatureProperties *)properties;

/**
 Adds a polygon geometry to the data source.

 @param polygon the polygon geometry to add to the data source
 @param properties the feature properties
 */
- (void)addPolygon:(TGGeoPolygon *)polygon withProperties:(TGFeatureProperties *)properties;

/**
 Adds a polyline to the data source.

 @param polyline the polyline geometry to add to the data source
 @param properties the feature properties
 */
- (void)addPolyline:(TGGeoPolyline *)polyline withProperties:(TGFeatureProperties *)properties;

/**
 Adds features described in a GeoJSON string to the data source. The string must be formatted according
 to the <a href="http://geojson.org/geojson-spec.html">GeoJSON specifications</a>.

 @param data the GeoJSON formatted string to add to the data source
 */
- (void)addGeoJson:(NSString *)data;

/**
 Clears the data source from all the added features.
 */
- (void)clear;

/**
 Definitely removes the data source from the map view.
 Any future usage of this `MapData` object will be a no-op.

 @return `YES` if removal was successful
 */
- (BOOL)remove;

/// The name of the data source
@property (readonly, nonatomic) NSString* name;

NS_ASSUME_NONNULL_END

/// The map view this data source is on
@property (readonly, weak, nonatomic) TGMapView* _Nullable mapView;

@end
