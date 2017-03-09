//
//  TGMarkerPickResult+Internal.h
//  TangramMap
//
//  Created by Karim Naaji on 2/27/17.
//  Copyright (c) 2017 Mapzen. All rights reserved.
//

#import "TGMarkerPickResult.h"

@interface TGMarkerPickResult ()

NS_ASSUME_NONNULL_BEGIN

/**
 Initializes a `TGMarkerPickResult`.

 @param coordinates the geographic coordinates of the label pick result
 @param marker the marker object selected

 @return an initialized `TGMarkerPickResult`

 @note You shouldn't have to create a `TGMarkerPickResult` yourself, those are returned as a result to
 a selection query on the `TGMapViewController` and initialized by the latter.
 */
- (instancetype) initWithCoordinates:(TGGeoPoint)coordinates marker:(TGMarker *)marker;

NS_ASSUME_NONNULL_END

@end

