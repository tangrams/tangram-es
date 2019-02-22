//
//  TGMapFeature.m
//  TangramMap
//
//  Created by Matt Blair on 2/21/19.
//

#import "TGMapFeature.h"

@implementation TGMapFeature

- (instancetype)initWithProperties:(TGFeatureProperties *)properties
{
    self = [super init];

    if (self) {
        _properties = properties;
    }

    return self;
}

@end

@implementation TGMapPointFeature

- (instancetype)initWithPoint:(CLLocationCoordinate2D)point properties:(TGFeatureProperties *)properties
{
    self = [super initWithProperties:properties];

    if (self) {
        _point = point;
    }

    return self;
}

@end

@implementation TGMapPolylineFeature

- (instancetype)initWithPolyline:(TGGeoPolyline *)polyline properties:(TGFeatureProperties *)properties
{
    self = [super initWithProperties:properties];

    if (self) {
        _polyline = polyline;
    }

    return self;
}

@end

@implementation TGMapPolygonFeature

- (instancetype)initWithPolygon:(TGGeoPolygon *)polygon properties:(TGFeatureProperties *)properties
{
    self = [super initWithProperties:properties];

    if (self) {
        _polygon = polygon;
    }

    return self;
}

@end
