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

/**
 Similar to `-[TGMarker point]` except that the point will transition to the
 geographic coordinate with a transition of time `seconds` and with an ease type function of type `ease`
 (See `TGEaseType`) from its previous coordinate, if a point geometry hasn't been set any coordinate yet,
 this method will act as `-[TGMarker point]`.

 @param coordinates the longitude and latitude where the marker will be placed
 @param seconds the animation duration given in seconds
 @param ease the ease function to be used between animation timestep
 @param error an error status pointer that will be assigned if the return status is NO

 @return YES if the operation succeeded, NO otherwise

 @note Markers can have their geometry set multiple time with possibly different geometry types.
 */
- (BOOL)pointEased:(TGGeoPoint)coordinates seconds:(float)seconds easeType:(TGEaseType)ease error:(NSError **)error;

/**
 Sets the styling for a marker with a string of YAML defining a 'draw rule'.

 See the more detailed scene <a href="https://mapzen.com/documentation/tangram/Styles-Overview/">documentation</a>
 to get more styling informations.

 @param styling the styling to set to this marker
 @param error an error status pointer that will be assigned if the return status is NO

 @return YES if the operation succeeded, NO otherwise

 @note Setting the stylingString will overwrite any previously set stylingString or stylingPath.
 */
- (BOOL)stylingString:(NSString *)styling error:(NSError **)error;

/**
 Sets the styling for a marker with a path, delimited by '.' that specifies a 'draw rule' in the
 current scene.

 See the more detailed scene <a href="https://mapzen.com/documentation/tangram/Styles-Overview/">documentation</a>
 to get more styling informations.

 @param path the styling path to set to this marker
 @param error an error status pointer that will be assigned if the return status is NO

 @return YES if the operation succeeded, NO otherwise

 @note Setting the stylingPath will overwrite any previously set stylingString or stylingPath.
 */
- (BOOL)stylingPath:(NSString *)path error:(NSError **)error;

/**
 Sets a marker to be a single point geometry at a geographic coordinate.

 @param coordinates the coordinates to set to this marker
 @param error an error status pointer that will be assigned if the return status is NO

 @return YES if the operation succeeded, NO otherwise

 @note Markers can have their geometry set multiple time with possibly different geometry types.
 */
- (BOOL)point:(TGGeoPoint)coordinates error:(NSError **)error;

/**
 Sets a marker styled to be a polyline (described in a `TGGeoPolyline`).

 @param polyline the polyline geometry to set to this marker
 @param error an error status pointer that will be assigned if non-nil and an error occured
 @param error an error status pointer that will be assigned if the return status is NO

 @return YES if the operation succeeded, NO otherwise

 @note Markers can have their geometry set multiple time with possibly different geometry types.
 */
- (BOOL)polyline:(TGGeoPolyline *)polyline error:(NSError **)error;

/**
 Sets a marker to be a polygon geometry (described in a `TGGeoPolygon`).

 @param polygon the polygon geometry to set to this marker
 @param error an error status pointer that will be assigned if the return status is NO

 @return YES if the operation succeeded, NO otherwise

 @note Markers can have their geometry set multiple time with possibly different geometry types.
 */
- (BOOL)polygon:(TGGeoPolygon *)polygon error:(NSError **)error;

/**
 Adjusts marker visibility

 @param visible whether the marker should be visible or not
 @param error an error status pointer that will be assigned if the return status is NO

 @return YES if the operation succeeded, NO otherwise
 */
- (BOOL)visible:(BOOL)visible error:(NSError **)error;

/**
 Set the ordering of point marker object relative to other markers; higher values are drawn 'above'.

 @param drawOrder the draw order to set to this marker
 @param error an error status pointer that will be assigned if the return status is NO

 @return YES if the operation succeeded, NO otherwise
 */
- (BOOL)drawOrder:(NSInteger)drawOrder error:(NSError **)error;

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

 @param icon the icon image to set to this marker
 @param error an error status pointer that will be assigned if the return status is NO

 @return YES if the operation succeeded, NO otherwise
 */
- (BOOL)icon:(UIImage *)icon error:(NSError **)error;

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
/// A custom user data
@property (assign, nonatomic) void* userData;

NS_ASSUME_NONNULL_END

@end
