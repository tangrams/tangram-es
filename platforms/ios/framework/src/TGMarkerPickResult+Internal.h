//
//  TGMarkerPickResult+Internal.h
//  TangramMap
//
//  Created by Karim Naaji on 2/27/17.
//  Updated by Matt Blair on 8/21/18.
//  Copyright (c) 2017 Mapzen. All rights reserved.
//

#import "TGMarkerPickResult.h"

@interface TGMarkerPickResult ()

NS_ASSUME_NONNULL_BEGIN

/**
 Initializes a `TGMarkerPickResult`.

 @param coordinate the geographic coordinates of the label pick result
 @param marker the marker object selected

 @return an initialized `TGMarkerPickResult`

 @note You shouldn't have to create a `TGMarkerPickResult` yourself, those are returned as a result to
 a selection query on the `TGMapView` and initialized by the latter.
 */
- (instancetype) initWithCoordinate:(CLLocationCoordinate2D)coordinate marker:(TGMarker *)marker;

NS_ASSUME_NONNULL_END

@end

