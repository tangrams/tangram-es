//
//  TGMapData.mm
//  TangramMap
//
//  Created by Karim Naaji on 2/24/16.
//  Copyright (c) 2017 Mapzen. All rights reserved.
//

#import "TGMapViewController+Internal.h"
#import <memory>
#import <vector>

typedef std::vector<Tangram::LngLat> Line;
typedef std::vector<Line> Polygon;

@interface TGMapData () {
    std::shared_ptr<Tangram::ClientGeoJsonSource> dataSource;
}

@property (copy, nonatomic) NSString* name;
@property (weak, nonatomic) TGMapViewController* mapView;

@end

static inline void tangramProperties(TGFeatureProperties* properties, Tangram::Properties& tgProperties)
{
    for (NSString* key in properties) {
        NSString* value = [properties objectForKey:key];
        tgProperties.set(std::string([key UTF8String]), [value UTF8String]);
    }
}

@implementation TGMapData

- (instancetype)initWithMapView:(TGMapViewController *)mapView name:(NSString *)name source:(std::shared_ptr<Tangram::ClientGeoJsonSource>)source
{
    self = [super init];

    if (self) {
        self.name = name;
        self.mapView = mapView;
        dataSource = source;
    }

    return self;
}

- (void)addPoint:(TGGeoPoint)coordinates withProperties:(TGFeatureProperties *)properties
{
    if (!self.mapView) {
        return;
    }

    Tangram::Properties tgProperties;
    tangramProperties(properties, tgProperties);

    Tangram::LngLat lngLat(coordinates.longitude, coordinates.latitude);
    dataSource->addPoint(tgProperties, lngLat);
}

- (void)addPolygon:(TGGeoPolygon *)polygon withProperties:(TGFeatureProperties *)properties
{
    if (!self.mapView) {
        return;
    }

    Tangram::Properties tgProperties;
    tangramProperties(properties, tgProperties);

    Polygon tgPolygon;
    Tangram::LngLat* coordinates = reinterpret_cast<Tangram::LngLat*>([polygon coordinates]);
    int* rings = reinterpret_cast<int*>([polygon rings]);

    size_t ringsCount = [polygon ringsCount];
    size_t ringStart = 0;
    size_t ringEnd = 0;

    for (size_t i = 0; i < ringsCount; ++i) {
        tgPolygon.emplace_back();
        auto& polygonRing = tgPolygon.back();

        ringEnd += rings[i];
        polygonRing.insert(polygonRing.begin(), coordinates + ringStart, coordinates + ringEnd);
        ringStart = ringEnd + 1;
    }

    dataSource->addPoly(tgProperties, tgPolygon);
}

- (void)addPolyline:(TGGeoPolyline *)polyline withProperties:(TGFeatureProperties *)properties
{
    if (!self.mapView) {
        return;
    }

    Tangram::Properties tgProperties;
    tangramProperties(properties, tgProperties);

    Line tgPolyline;
    Tangram::LngLat* coordinates = reinterpret_cast<Tangram::LngLat*>([polyline coordinates]);
    tgPolyline.insert(tgPolyline.begin(), coordinates, coordinates + [polyline count]);
    dataSource->addLine(tgProperties, tgPolyline);
}

- (void)addGeoJson:(NSString *)data
{
    if (!self.mapView) {
        return;
    }

    std::string sourceData = std::string([data UTF8String]);
    dataSource->addData(sourceData);
}

- (void)clear
{
    if (!self.mapView) {
        return;
    }

    [self.mapView clearDataSource:dataSource];
}

- (void)remove
{
    if (!self.mapView) {
        return;
    }

    [self.mapView removeDataSource:dataSource name:self.name];
    self.mapView = nil;
}

@end
