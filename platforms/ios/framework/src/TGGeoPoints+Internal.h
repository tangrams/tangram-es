//
//  TGGeoPoints+Internal.h
//  TangramMap
//
//  Created by Matt Blair on 2/21/19.
//  Copyright (c) 2019 Mapzen. All rights reserved.
//

#import "TGGeoPoints.h"

NS_ASSUME_NONNULL_BEGIN

TG_EXPORT
@interface TGGeoPoints ()

- (instancetype)initWithCoordinates:(const CLLocationCoordinate2D *)coordinates count:(NSUInteger)count;

@end

NS_ASSUME_NONNULL_END
