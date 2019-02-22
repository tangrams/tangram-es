//
//  TGGeoPoints.h
//  TangramMap
//
//  Created by Matt Blair on 2/21/19.
//  Copyright (c) 2019 Mapzen. All rights reserved.
//

#import <CoreLocation/CoreLocation.h>
#import <Foundation/Foundation.h>
#import "TGExport.h"

NS_ASSUME_NONNULL_BEGIN

TG_EXPORT
@interface TGGeoPoints : NSObject

/**
 Gets a pointer to the geographic coordinates describing this poyline of array length `-[TGGeoPolyline count:]`.

 @return a pointer to the list of geographic coordinates describing this polyline
 */
@property(readonly) CLLocationCoordinate2D *coordinates;

/**
 Gets the number of geographic coordinates describing this polyline.

 @return the number of geographic coordinates in this polyline
 */
@property(readonly, nonatomic) NSUInteger count;

@end

NS_ASSUME_NONNULL_END
