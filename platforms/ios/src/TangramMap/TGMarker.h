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

@interface TGMarker : NSObject

/*
 TODO
 */
- (instancetype)init;

/**
 Similar to `-[TGMapViewController markerSetPoint:coordinates:]` except that the point will transition to the
 geographic coordinate with a transition of time `seconds` and with an ease type function of type `ease`
 (See `TGEaseType`) from its previous coordinate, if a point geometry hasn't been set any coordinate yet,
 this method will act as `-markerSetPoint:coordinates:`.

 @param identifier the marker identifier created with `-markerAdd`
 @param the longitude and latitude where the marker will be placed
 @param seconds the animation duration given in seconds
 @param ease the ease function to be used between animation timestep
 @return `YES` if this operation was successful, `NO` otherwise

 @note Markers can have their geometry set multiple time with possibly different geometry types.
 */
- (BOOL)setPointEased:(TGGeoPoint)coordinates seconds:(float)seconds easeType:(TGEaseType)ease;

/**
 Sets the styling for a marker identifier.

 See the more detailed scene <a href="https://mapzen.com/documentation/tangram/Styles-Overview/">documentation</a>
 to get more styling informations.

 @param identifier the marker identifier created with `-markerAdd`
 @param styling the styling string in YAML syntax to set to the marker

 @return `YES` if this operation was successful, `NO` otherwise

 @note It is possible to update the marker styling multiple times.
 */
@property (copy, nonatomic) NSString* styling;

/**
 Sets a marker to be a single point geometry at a geographic coordinate.

 @param identifier the marker identifier created with `-markerAdd`
 @param the longitude and latitude where the marker will be placed
 @return `YES` if this operation was successful, `NO` otherwise

 @note Markers can have their geometry set multiple time with possibly different geometry types.
 */
@property (assign, nonatomic) TGGeoPoint point;

/**
 Sets a marker styled to be a polyline (described in a `TGGeoPolyline`).

 @param identifier the marker identifier created with `-markerAdd`
 @oaram polyline the polyline geometry to use for this marker
 @return `YES` if this operation was successful, `NO` otherwise

 @note Markers can have their geometry set multiple time wwith possibly different geometry types.
 */
@property (strong, nonatomic) TGGeoPolyline* polyline;

/**
 Sets a marker to be a polygon geometry (described in a `TGGeoPolygon`).

 @param identifier the marker identifier created with `-[TGMapViewController markerAdd]`
 @param polygon the polygon geometry to use for this marker
 @return `YES` if this operation was successful, `NO` otherwise

 @note Markers can have their geometry set multiple time with possibly different geometry types.
 */
@property (strong, nonatomic) TGGeoPolygon* polygon;

/**
 Adjusts marker visibility

 @param identifier the marker identifier created with `- markerAdd`
 @param whether the marker is set to visible
 @return `YES` if this operation was successful, `NO` otherwise
 */
@property (assign, nonatomic) BOOL visible;

/**
 Sets an image loaded with a <a href="https://developer.apple.com/reference/uikit/uiimage">
 UIImage</a> to a marker styled with a <a href="https://mapzen.com/documentation/tangram/Styles-Overview/#points">
 points style</a>.

 ```swift
 var identifier = view.markerAdd()
 view.markerSetStyling(identifier, styling: "{ style: 'points', color: 'white', size: [25px, 25px], order:500 }")
 view.markerSetPoint(identifier, coordinates: TGGeoPointMake(longitude, latitude))
 view.markerSetImage(identifier, image: UIIMage(name: "marker-image.png"))
 ```

 @param identifier the marker identifier created with `-markerAdd`
 @param the `UIImage` that will be used to be displayed on the marker
 @return `YES` if this operation was successful, `NO` otherwise

 @note An image marker must be styled with a
 <a href="https://mapzen.com/documentation/tangram/Styles-Overview/#points">point style</a>.
 */
@property (strong, nonatomic) UIImage * icon;

/*
 TODO
 */
@property (weak, nonatomic) TGMapViewController* map;

NS_ASSUME_NONNULL_END

@end
