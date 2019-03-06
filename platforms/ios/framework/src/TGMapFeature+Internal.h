//
//  TGMapFeature+Internal.h
//  tangram
//
//  Created by Matt Blair on 2/25/19.
//

#import "TGMapFeature.h"

#pragma mark Common Map Feature Behavior

@interface TGMapFeature ()

- (instancetype)initWithProperties:(TGFeatureProperties *)properties;

@end

#pragma mark Point Feature

@interface TGMapPointFeature : TGMapFeature

- (instancetype)initWithPoint:(CLLocationCoordinate2D)point properties:(TGFeatureProperties *)properties;

@end

#pragma mark Polyline Feature

@interface TGMapPolylineFeature : TGMapFeature

- (instancetype)initWithPolyline:(TGGeoPolyline *)polyline properties:(TGFeatureProperties *)properties;

@end

#pragma mark Polygon Feature

@interface TGMapPolygonFeature : TGMapFeature

- (instancetype)initWithPolygon:(TGGeoPolygon *)polygon properties:(TGFeatureProperties *)properties;

@end
