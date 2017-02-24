//
//  TGMapData.h
//  TangramMap
//
//  Created by Karim Naaji on 2/24/16.
//  Copyright (c) 2017 Mapzen. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "TGGeoPoint.h"
#import "TGGeoPolygon.h"
#import "TGGeoPolyline.h"

@interface TGMapData : NSObject

typedef NSDictionary<NSString *, NSString *> FeatureProperties;

NS_ASSUME_NONNULL_BEGIN

- (void)addPoint:(TGGeoPoint)coordinates withProperties:(FeatureProperties *)properties;

- (void)addPolygon:(TGGeoPolygon *)polygon withProperties:(FeatureProperties *)properties;

- (void)addPolyline:(TGGeoPolyline *)polyline withProperties:(FeatureProperties *)properties;

- (void)addGeoJson:(NSString *)data;

- (void)clear;

@property (readonly, nonatomic) TGMapViewController* map;
@property (readonly, nonatomic) NSString* name;

NS_ASSUME_NONNULL_END

@end
