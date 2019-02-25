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
#import "TGMapFeature.h"

@class TGMapView;
@class TGGeoPolygon;
@class TGGeoPolyline;

/**
 A `TGMapData` enables you to dynamically create map features to be rendered in the current map scene.

 To render features from a `TGMapData` in your map scene, add a layer in your scene file with a data source set to a new
 source name. This source name will correspond to the name of your `TGMapData` instance.

 Example:
 ```yaml
 layers:
     route_line_transit:
        data: { source: route_line_transit_data }
        draw:
            lines:
                color: function() { return feature.color || '#06a6d4'; }
                order: 500
                width: 10px
 ```

 In your application, create a `TGMapData` instance with the same name using
 `-[TGMapView addDataLayer:generateCentroid:]` and assign map features to it.

 Example (Objective-C):
 ```
 CLLocationCoordinate2D polylinePoints[] = { ... };
 TGGeoPolyline *polyline = [[TGGeoPolyline alloc] initWithCoordinates:polylinePoints count:polylinePointCount];
 TGFeatureProperties *properties = @{ @"type" : @"line", @"color" : @"#D2655F" };
 TGMapFeature *feature = [TGMapFeature mapFeatureWithPolyline:polyline properties:properties];
 TGMapData *mapData = [mapView addDataLayer:@"route_line_transit_data", generateCentroid:NO];
 [mapData setFeatures:@[feature]];
 ```

 Example (Swift):
 ```swift
 var polyline = TGGeoPolyline(coordinates: polylinePoints, count: polylinePointCount);
 var properties = ["type": "line", "color": "#D2655F"];
 var feature = TGMapFeature(polyline: polyline, properties: properties);
 var mapData = mapView.addDataLayer(name: "mz_route_line_transit_data");
 mapData.setFeatures([feature]);
 ```
 */
TG_EXPORT
@interface TGMapData : NSObject

NS_ASSUME_NONNULL_BEGIN

/**
 Sets the contents of this map data to the specified array of features.

 This replaces any previously assigned contents.

 @param features The array of features to assign.
 */
- (void)setFeatures:(NSArray<TGMapFeature *> *)features;

/**
 Sets the contents of this map data to the features defined in a
 <a href="http://geojson.org/geojson-spec.html">GeoJSON</a> string.

 This replaces any previously assigned contents.

 @param data A GeoJSON-formatted string.
 */
- (void)setGeoJson:(NSString *)data;

/**
 Permanently removes this map data from the map view.

 Any future use of this object will do nothing.

 @return `YES` if removal was successful.
 */
- (BOOL)remove;

/// The name of the data source.
@property (readonly, nonatomic) NSString* name;

NS_ASSUME_NONNULL_END

/// The map view that created this map data.
@property (readonly, weak, nonatomic) TGMapView* _Nullable mapView;

@end
