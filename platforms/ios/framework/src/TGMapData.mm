//
//  TGMapData.mm
//  TangramMap
//
//  Created by Karim Naaji on 2/24/16.
//  Updated by Matt Blair on 8/21/18.
//  Copyright (c) 2017 Mapzen. All rights reserved.
//

#import "TGGeoPolygon.h"
#import "TGGeoPolyline.h"
#import "TGMapData.h"
#import "TGMapData+Internal.h"
#import "TGMapView.h"
#import "TGMapView+Internal.h"
#import "TGTypes+Internal.h"

#include "tangram.h"
#include <memory>
#include <vector>

typedef std::vector<Tangram::LngLat> Line;
typedef std::vector<Line> Polygon;

@interface TGMapData () {
    std::shared_ptr<Tangram::ClientGeoJsonSource> dataSource;
}

@property (copy, nonatomic) NSString* name;
@property (weak, nonatomic) TGMapView* map;

@end

static inline void tangramProperties(TGFeatureProperties* properties, Tangram::Properties& tgProperties)
{
    for (NSString* key in properties) {
        NSString* value = [properties objectForKey:key];
        tgProperties.set(std::string([key UTF8String]), [value UTF8String]);
    }
}

@implementation TGMapData

- (instancetype)initWithMapView:(__weak TGMapView *)mapView name:(NSString *)name source:(std::shared_ptr<Tangram::ClientGeoJsonSource>)source
{
    self = [super init];

    if (self) {
        self.name = name;
        self.map = mapView;
        dataSource = source;
    }

    return self;
}

- (void)addPoint:(CLLocationCoordinate2D)point withProperties:(TGFeatureProperties *)properties
{
    if (!self.map) {
        return;
    }

    Tangram::Properties tgProperties;
    tangramProperties(properties, tgProperties);

    Tangram::LngLat lngLat(point.longitude, point.latitude);
    dataSource->addPoint(tgProperties, lngLat);
}

- (void)addPolygon:(TGGeoPolygon *)polygon withProperties:(TGFeatureProperties *)properties
{
    if (!self.map) {
        return;
    }

    Tangram::Properties tgProperties;
    tangramProperties(properties, tgProperties);

    Polygon tgPolygon;
    CLLocationCoordinate2D* coordinates = polygon.coordinates;
    int* rings = polygon.rings;
    size_t ringsCount = polygon.ringsCount;
    size_t ringStart = 0;
    size_t ringEnd = 0;

    for (size_t i = 0; i < ringsCount; ++i) {
        tgPolygon.emplace_back();
        auto& polygonRing = tgPolygon.back();

        ringEnd += rings[i];
        for (auto i = ringStart; i < ringEnd; i++) {
            polygonRing.push_back(TGConvertCLLocationCoordinate2DToCoreLngLat(coordinates[i]));
        }
        ringStart = ringEnd + 1;
    }

    dataSource->addPoly(tgProperties, tgPolygon);
}

- (void)addPolyline:(TGGeoPolyline *)polyline withProperties:(TGFeatureProperties *)properties
{
    if (!self.map) {
        return;
    }

    Tangram::Properties tgProperties;
    tangramProperties(properties, tgProperties);

    Line tgPolyline;
    CLLocationCoordinate2D* coordinates = polyline.coordinates;
    size_t count = polyline.count;
    for (size_t i = 0; i < count; i++) {
        tgPolyline.push_back(TGConvertCLLocationCoordinate2DToCoreLngLat(coordinates[i]));
    }
    dataSource->addLine(tgProperties, tgPolyline);
}

- (void)addGeoJson:(NSString *)data
{
    if (!self.map) {
        return;
    }

    std::string sourceData = std::string([data UTF8String]);
    dataSource->addData(sourceData);
}

- (void)clear
{
    if (!self.map) {
        return;
    }

    [self.map clearDataSource:dataSource];
}

- (BOOL)remove
{
    if (!self.map) {
        return NO;
    }

    BOOL removed = [self.map removeDataSource:dataSource name:self.name];
    self.map = nil;
    return removed;
}

@end
