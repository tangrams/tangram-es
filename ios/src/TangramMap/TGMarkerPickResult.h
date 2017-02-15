//
//  TGMarkerPickResult.h
//  TangramMap
//
//  Created by Karim Naaji on 01/03/17.
//  Copyright (c) 2017 Mapzen. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "TGGeoPoint.h"

NS_ASSUME_NONNULL_BEGIN

/**
 A marker identifier for use in the <a href="Classes/TGMapViewController.html#/Marker%20interface">marker interface</a>.
 A marker identifier of 0 is non-valid.
 */
typedef uint32_t TGMapMarkerId;

/**
 Data structure holding the result of a marker selection that occured on the map view.

 See `-[TGMapViewController pickMarkerAt:]` and `[TGMapViewDelegate mapView:didSelectMarker:atScreenPosition:]`.
 */
@interface TGMarkerPickResult : NSObject

/**
 Initializes a `TGMarkerPickResult`.

 @param coordinates the geographic coordinates of the label pick result
 @param identifier the marker identifier

 @return an initialized `TGMarkerPickResult`

 @note You shouldn't have to create a `TGMarkerPickResult` yourself, those are returned as a result to
 a selection query on the `TGMapViewController` and initialized by the latter.
 */
- (instancetype) initWithCoordinates:(TGGeoPoint)coordinates identifier:(TGMapMarkerId)identifier;

/// The geographic coordinates of the selected label
@property (readonly, nonatomic) TGGeoPoint coordinates;

/// The identifier of the selected marker
@property (readonly, nonatomic) TGMapMarkerId identifier;

NS_ASSUME_NONNULL_END

@end
