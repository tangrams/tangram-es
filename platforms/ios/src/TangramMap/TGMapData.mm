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

@interface TGMapData () {
    std::shared_ptr<Tangram::ClientGeoJsonSource> dataSource;
}

@property (copy, nonatomic) NSString* name;
@property (weak, nonatomic) TGMapViewController* mapView;

@end

static inline void tangramProperties(FeatureProperties* properties, Tangram::Properties& tgProperties)
{
    for (NSString* key in properties) {
        NSString* value = [properties objectForKey:key];
        tgProperties.set(std::string([key UTF8String]), [value UTF8String]);
    }
}

@implementation TGMapData

- (instancetype)initWithMapView:(TGMapViewController *)mapView name:(NSString *)name
{
    self = [super init];

    if (self) {
        self.name = name;
        self.mapView = mapView;

        // Create client data source with Tangram
        dataSource = [self.mapView createDataSource:name];
    }

    return self;
}

- (void)addPoint:(TGGeoPoint)coordinates withProperties:(FeatureProperties *)properties
{
    Tangram::Properties tgProperties;
    tangramProperties(properties, tgProperties);

    Tangram::LngLat lngLat(coordinates.longitude, coordinates.latitude);
    dataSource->addPoint(tgProperties, lngLat);
}

- (void)addPolygon:(TGGeoPolygon *)polygon withProperties:(FeatureProperties *)properties
{
    Tangram::Properties tgProperties;
    tangramProperties(properties, tgProperties);

    std::vector<std::vector<Tangram::LngLat>> tgPolygon;
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

- (void)addPolyline:(TGGeoPolyline *)polyline withProperties:(FeatureProperties *)properties
{
    Tangram::Properties tgProperties;
    tangramProperties(properties, tgProperties);

    std::vector<Tangram::LngLat> tgPolyline;
    Tangram::LngLat* coordinates = reinterpret_cast<Tangram::LngLat*>([polyline coordinates]);
    tgPolyline.insert(tgPolyline.begin(), coordinates, coordinates + [polyline count]);
    dataSource->addLine(tgProperties, tgPolyline);
}

- (void)addGeoJson:(NSString *)data
{
    std::string sourceData = std::string([data UTF8String]);
    dataSource->addData(sourceData);
}

- (void)clear
{
    [self.mapView clearDataSource:dataSource];
}

@end
