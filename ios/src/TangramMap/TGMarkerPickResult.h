//
//  TGMarkerPickResult.h
//  tangram
//
//  Created by Karim Naaji on 01/03/17.
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

- (instancetype) initWithCoordinates:(TGGeoPoint)coordinates identifier:(TGMapMarkerId)identifier;

/// The geographic coordinates of the selected label
@property (readonly, nonatomic) TGGeoPoint coordinates;

/// The identifier of the selected marker
@property (readonly, nonatomic) TGMapMarkerId identifier;

NS_ASSUME_NONNULL_END

@end
