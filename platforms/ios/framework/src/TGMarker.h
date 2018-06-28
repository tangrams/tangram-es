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
#import "TGTypes.h"

@class TGMapViewController;

NS_ASSUME_NONNULL_BEGIN

/**
 Marker interface that makes you able to add icons, polylines and polygons to the map view.

 The marker style should defined using the <a href="https://mapzen.com/documentation/tangram/Styles-Overview">
 Tangram YAML syntax</a>.
 */
@interface TGMarker : NSObject

/**
 Similar to `-[TGMarker point]` except that the point will transition to the
 geographic coordinate with a transition of time `seconds` and with an ease type function of type `ease`
 (See `TGEaseType`) from its previous coordinate, if a point geometry hasn't been set any coordinate yet,
 this method will act as `-[TGMarker point]`.

 @param coordinates the longitude and latitude where the marker will be placed
 @param seconds the animation duration given in seconds
 @param ease the ease function to be used between animation timestep

 @note Markers can have their geometry set multiple time with possibly different geometry types.
 */
- (void)pointEased:(TGGeoPoint)coordinates seconds:(float)seconds easeType:(TGEaseType)ease;

/**
 Sets the styling for a marker with a string of YAML defining a 'draw rule'.
 See the more detailed scene <a href="https://mapzen.com/documentation/tangram/Styles-Overview/">documentation</a>
 to get more styling informations.

 @note Setting the stylingString will overwrite any previously set stylingString or stylingPath.
 */
@property (copy, nonatomic) NSString* stylingString;

/**
 Sets the styling for a marker with a path, delimited by '.' that specifies a 'draw rule' in the
 current scene.
 See the more detailed scene <a href="https://mapzen.com/documentation/tangram/Styles-Overview/">documentation</a>
 to get more styling informations.

 @note Setting the stylingPath will overwrite any previously set stylingString or stylingPath.
 */
@property (copy, nonatomic) NSString* stylingPath;

/**
 Sets a marker to be a single point geometry at a geographic coordinate.

 @note Markers can have their geometry set multiple time with possibly different geometry types.
 */
@property (assign, nonatomic) TGGeoPoint point;

/**
 Sets a marker styled to be a polyline (described in a `TGGeoPolyline`).

 @note Markers can have their geometry set multiple time wwith possibly different geometry types.
 */
@property (strong, nonatomic) TGGeoPolyline* polyline;

/**
 Sets a marker to be a polygon geometry (described in a `TGGeoPolygon`).

 @note Markers can have their geometry set multiple time with possibly different geometry types.
 */
@property (strong, nonatomic) TGGeoPolygon* polygon;

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
 */
@property (strong, nonatomic) UIImage* icon;

/// Adjusts marker visibility
@property (assign, nonatomic) BOOL visible;
/// Set the ordering of point marker object relative to other markers; higher values are drawn 'above'
@property (assign, nonatomic) NSInteger drawOrder;
/// A custom user data
@property (assign, nonatomic) void* userData;

NS_ASSUME_NONNULL_END

/**
 Returns whether the usage of this marker resulted in internal errors

 @param error a pointer to output error
 @return YES if an error has been set to the input paramater, NO otherwise

 @note The internal marker error will remain until you access it,
 make sure to check for errors on the marker after critical API calls or property access.
 */
- (BOOL)getError:(NSError* _Nullable * _Nullable)error;

@end
