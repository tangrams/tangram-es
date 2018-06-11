//
//  TGMarkerPickResult.h
//  TangramMap
//
//  Created by Karim Naaji on 01/03/17.
//  Copyright (c) 2017 Mapzen. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "TGGeoPoint.h"
#import "TGMarker.h"

NS_ASSUME_NONNULL_BEGIN

/**
 Data structure holding the result of a marker selection that occured on the map view.

 See `-[TGMapViewController pickMarkerAt:]` and `[TGMapViewDelegate mapView:didSelectMarker:atScreenPosition:]`.
 */
@interface TGMarkerPickResult : NSObject

/// The geographic coordinates of the selected label
@property (readonly, nonatomic) TGGeoPoint coordinates;

/// The selected marker
@property (readonly, nonatomic) TGMarker* marker;

NS_ASSUME_NONNULL_END

@end
