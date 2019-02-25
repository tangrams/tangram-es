//
//  TGMapFeature.m
//  TangramMap
//
//  Created by Matt Blair on 2/21/19.
//

#import "TGMapFeature+Internal.h"

@implementation TGMapFeature

- (instancetype)initWithProperties:(TGFeatureProperties *)properties
{
    self = [super init];
    if (self) {
        _properties = properties;
    }
    return self;
}

- (CLLocationCoordinate2D *)point
{
    return nil;
}

- (TGGeoPolyline *)polyline
{
    return nil;
}

- (TGGeoPolygon *)polygon
{
    return nil;
}

+ (instancetype)mapFeatureWithPoint:(CLLocationCoordinate2D)point properties:(TGFeatureProperties *)properties
{
    return [[TGMapPointFeature alloc] initWithPoint:point properties:properties];
}

+ (instancetype)mapFeatureWithPolyline:(TGGeoPolyline *)polyline properties:(TGFeatureProperties *)properties
{
    return [[TGMapPolylineFeature alloc] initWithPolyline:polyline properties:properties];
}

+ (instancetype)mapFeatureWithPolygon:(TGGeoPolygon *)polygon properties:(TGFeatureProperties *)properties
{
    return [[TGMapPolygonFeature alloc] initWithPolygon:polygon properties:properties];
}

@end

#pragma mark Point Feature

@implementation TGMapPointFeature {
    CLLocationCoordinate2D _point;
}

- (instancetype)initWithPoint:(CLLocationCoordinate2D)point properties:(TGFeatureProperties *)properties
{
    self = [super initWithProperties:properties];
    if (self) {
        _point = point;
    }
    return self;
}

- (CLLocationCoordinate2D *)point
{
    return &_point;
}

@end

#pragma mark Polyline Feature

@implementation TGMapPolylineFeature {
    TGGeoPolyline *_polyline;
}

- (instancetype)initWithPolyline:(TGGeoPolyline *)polyline properties:(TGFeatureProperties *)properties
{
    self = [super initWithProperties:properties];
    if (self) {
        _polyline = polyline;
    }
    return self;
}

- (TGGeoPolyline *)polyline
{
    return _polyline;
}

@end

#pragma mark Polygon Feature

@implementation TGMapPolygonFeature {
    TGGeoPolygon *_polygon;
}

- (instancetype)initWithPolygon:(TGGeoPolygon *)polygon properties:(TGFeatureProperties *)properties
{
    self = [super initWithProperties:properties];
    if (self) {
        _polygon = polygon;
    }
    return self;
}

- (TGGeoPolygon *)polygon
{
    return _polygon;
}

@end
