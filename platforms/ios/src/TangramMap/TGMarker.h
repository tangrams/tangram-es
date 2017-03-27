//
//  TGMarker.h
//  TangramMap
//
//  Created by Karim Naaji on 2/17/17.
//  Copyright (c) 2017 Mapzen. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

#import "TGGeoPoint.h"
#import "TGGeoPolygon.h"
#import "TGGeoPolyline.h"
#import "TGEaseType.h"

@class TGMapViewController;

NS_ASSUME_NONNULL_BEGIN

/**
 Marker interface that makes you able to add icons, polylines and polygons to the map view.

 The marker style should defined using the <a href="https://mapzen.com/documentation/tangram/Styles-Overview">
 Tangram YAML syntax</a>.
 */
@interface TGMarker : NSObject

/*
 Initializes a `TGMarker`.

 @note the marker will not be visible on any map view, if you want to add this marker to a view make sure
 to set a map view to this marker object with `-[TGMarker map]`, style it and set its geometry.
 */
- (instancetype)init;

/*
 Initializes a `TGMarker` and adds it to the map view.

 @param mapView the map view to add this marker to

 @note the marker will not be visible on the map view until you set its styling and set its geometry.
 */
- (instancetype)initWithMapView:(TGMapViewController *)mapView;

/**
 Similar to `-[TGMarker point]` except that the point will transition to the
 geographic coordinate with a transition of time `seconds` and with an ease type function of type `ease`
 (See `TGEaseType`) from its previous coordinate, if a point geometry hasn't been set any coordinate yet,
 this method will act as `-[TGMarker point]`.

 @param coordinates the longitude and latitude where the marker will be placed
 @param seconds the animation duration given in seconds
 @param ease the ease function to be used between animation timestep
 @return `YES` if this operation was successful, `NO` otherwise

 @note Markers can have their geometry set multiple time with possibly different geometry types.
 */
- (NSError *)pointEased:(TGGeoPoint)coordinates seconds:(float)seconds easeType:(TGEaseType)ease;

/**
 Sets the styling for a marker with a string of YAML defining a 'draw rule'.

 See the more detailed scene <a href="https://mapzen.com/documentation/tangram/Styles-Overview/">documentation</a>
 to get more styling informations.

 @note Setting the stylingString will overwrite any previously set stylingString or stylingPath.

 @return An error if the marker can't be shown on the map
 */
- (NSError *)stylingString:(NSString *)styling;

/**
 Sets the styling for a marker with a path, delimited by '.' that specifies a 'draw rule' in the
 current scene.

 See the more detailed scene <a href="https://mapzen.com/documentation/tangram/Styles-Overview/">documentation</a>
 to get more styling informations.

 @note Setting the stylingPath will overwrite any previously set stylingString or stylingPath.

 @return An error if the marker can't be shown on the map
 */
- (NSError *)stylingPath:(NSString *)path;

/**
 Sets a marker to be a single point geometry at a geographic coordinate.

 @note Markers can have their geometry set multiple time with possibly different geometry types.

 @return An error if the marker can't be shown on the map
 */
- (NSError *)point:(TGGeoPoint)coordinates;

/**
 Sets a marker styled to be a polyline (described in a `TGGeoPolyline`).

 @note Markers can have their geometry set multiple time wwith possibly different geometry types.

 @return An error if the marker can't be shown on the map
 */
- (NSError *)polyline:(TGGeoPolyline *)polyline;

/**
 Sets a marker to be a polygon geometry (described in a `TGGeoPolygon`).

 @note Markers can have their geometry set multiple time with possibly different geometry types.

 @return An error if the marker can't be shown on the map
 */
- (NSError *)polygon:(TGGeoPolygon *)polygon;

/**
 Adjusts marker visibility
 
 @return An error if the marker can't be shown on the map
 */
- (NSError *)visible:(BOOL)visible;

/**
 Set the ordering of point marker object relative to other markers; higher values are drawn 'above'.

 @return An error if the marker can't be shown on the map
 */
- (NSError *)drawOrder:(NSInteger)drawOrder;

/**
 Sets an icon loaded with a <a href="https://developer.apple.com/reference/uikit/uiimage">
 UIImage</a> to a marker styled with a <a href="https://mapzen.com/documentation/tangram/Styles-Overview/#points">
 points style</a>.

 ```swift
 TGMarker marker;
 marker.styling = "{ style: 'points', color: 'white', size: [25px, 25px], order:500 }"
 marker.point = TGGeoPointMake(longitude, latitude)
 marker.icon = UIIMage(name: "marker-icon.png")
 ```

 @note An icon marker must be styled with a
 <a href="https://mapzen.com/documentation/tangram/Styles-Overview/#points">point style</a>.

 @return An error if the marker can't be shown on the map
 */
- (NSError *)icon:(UIImage *)icon;

/// Access the marker styling string (readonly)
@property (readonly, nonatomic) NSString* stylingString;
/// Access the marker styling path (readonly)
@property (readonly, nonatomic) NSString* stylingPath;
/// Access the marker coordinate (readonly)
@property (readonly, nonatomic) TGGeoPoint point;
/// Access the marker polyline (readonly)
@property (readonly, nonatomic) TGGeoPolyline* polyline;
/// Access the marker polygon (readonly)
@property (readonly, nonatomic) TGGeoPolygon* polygon;
/// Access whether the marker visibility (readonly)
@property (readonly, nonatomic) BOOL visible;
/// Access the marker draw order (readonly)
@property (readonly, nonatomic) NSInteger drawOrder;
/// Access the marker icon (readonly)
@property (readonly, nonatomic) UIImage* icon;

NS_ASSUME_NONNULL_END

/*
 The map this marker is on.
 Setting the map view will add the marker to the map, and setting it to `nil`
 will remove the marker from it.

 A marker can be only active at at most one `TGMapViewController` at a time.

 @return An error if the marker can't be shown on the map
 */
- (NSError * _Nonnull)map:(nullable TGMapViewController *)mapView;

/// Access the marker map view (readonly)
@property (readonly, nonatomic) TGMapViewController* _Nullable map;

@end
