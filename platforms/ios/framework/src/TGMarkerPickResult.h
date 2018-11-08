//
//  TGMarkerPickResult.h
//  TangramMap
//
//  Created by Karim Naaji on 01/03/17.
//  Copyright (c) 2017 Mapzen. All rights reserved.
//

#import <CoreLocation/CoreLocation.h>
#import <Foundation/Foundation.h>
#import "TGExport.h"

@class TGMarker;

NS_ASSUME_NONNULL_BEGIN

/**
 Data structure holding the result of a marker selection that occured on the map view.

 See `-[TGMapView pickMarkerAt:]` and `[TGMapViewDelegate mapView:didSelectMarker:atScreenPosition:]`.
 */
TG_EXPORT
@interface TGMarkerPickResult : NSObject

/// The geographic coordinates of the selected label
@property (readonly, nonatomic) CLLocationCoordinate2D coordinates;

/// The selected marker
@property (readonly, nonatomic) TGMarker* marker;

NS_ASSUME_NONNULL_END

@end
