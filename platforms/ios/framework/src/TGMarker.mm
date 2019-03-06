//
//  TGMarker.mm
//  TangramMap
//
//  Created by Karim Naaji on 2/17/17.
//  Updated by Matt Blair on 7/13/18.
//  Copyright (c) 2017 Mapzen. All rights reserved.
//

#import "TGGeoPolygon.h"
#import "TGGeoPolyline.h"
#import "TGMarker.h"
#import "TGMarker+Internal.h"
#import "TGMapView.h"
#import "TGMapView+Internal.h"
#import "TGTypes.h"
#import "TGTypes+Internal.h"

#include "map.h"

@interface TGMarker () {
    Tangram::Map* tangramInstance;
    NSError* error;
}

- (void)createNSError;
- (void)clearGeometry;

@end

@implementation TGMarker

- (instancetype)initWithMap:(Tangram::Map*)map
{
    self = [super init];

    if (self) {
        tangramInstance = map;
        self.identifier = tangramInstance->markerAdd();
        self.visible = YES;
        self.drawOrder = 0;
    }

    return self;
}

- (void)clearGeometry
{
    _polyline = nil;
    _polygon = nil;
    _point = {NAN, NAN};
}

- (void)createNSError
{
    // TODO: Check for all Tangram internal tangram error and
    // convert it to this error string for additional infos
    NSString* errorString = @"Tangram marker internal error";

    self->error = [NSError errorWithDomain:TGErrorDomain code:TGErrorMarker userInfo:@{ NSLocalizedDescriptionKey:errorString }];
}

- (void)setStylingString:(NSString *)styling
{
    _stylingString = styling;
    _stylingPath = nil;

    if (!tangramInstance->markerSetStylingFromString(self.identifier, [styling UTF8String])) {
        [self createNSError];
    }
}

- (void)setStylingPath:(NSString *)path
{
    _stylingPath = path;
    _stylingString = nil;

    if (!tangramInstance->markerSetStylingFromPath(self.identifier, [path UTF8String])) {
        [self createNSError];
    }
}

- (void)pointEased:(CLLocationCoordinate2D)point seconds:(NSTimeInterval)seconds easeType:(TGEaseType)ease
{
    [self clearGeometry];
    _point = point;

    Tangram::LngLat lngLat(point.longitude, point.latitude);

    if (!tangramInstance->markerSetPointEased(self.identifier, lngLat, seconds, TGConvertTGEaseTypeToCoreEaseType(ease))) {
        [self createNSError];
    }
}

- (void)setPoint:(CLLocationCoordinate2D)point
{
    [self clearGeometry];
    _point = point;

    Tangram::LngLat lngLat(point.longitude, point.latitude);

    if (!tangramInstance->markerSetPoint(self.identifier, lngLat)) {
        [self createNSError];
    }
}

- (void)setPolyline:(TGGeoPolyline *)polyline
{
    [self clearGeometry];
    _polyline = polyline;

    CLLocationCoordinate2D *coordinates = polyline.coordinates;
    size_t count = polyline.count;
    std::vector<Tangram::LngLat> lngLatCoords(count);
    for (size_t i = 0; i < count; i++) {
        lngLatCoords[i] = TGConvertCLLocationCoordinate2DToCoreLngLat(coordinates[i]);
    }

    if (!tangramInstance->markerSetPolyline(self.identifier, lngLatCoords.data(), static_cast<int>(polyline.count))) {
        [self createNSError];
    }
}

- (void)setPolygon:(TGGeoPolygon *)polygon
{
    [self clearGeometry];
    _polygon = polygon;

    std::vector<Tangram::LngLat> coordinates;
    std::vector<int> ringCounts;
    for (TGGeoPolyline *ring in polygon.rings) {
        ringCounts.push_back(static_cast<int>(ring.count));
        size_t numberOfPoints = ring.count;
        for (size_t i = 0; i < numberOfPoints; i++) {
            coordinates.push_back(TGConvertCLLocationCoordinate2DToCoreLngLat(ring.coordinates[i]));
        }
    }

    if (!tangramInstance->markerSetPolygon(self.identifier, coordinates.data(), ringCounts.data(), static_cast<int>(ringCounts.size()))) {
        [self createNSError];
    }
}

- (void)setVisible:(BOOL)visible
{
    _visible = visible;

    if (!tangramInstance->markerSetVisible(self.identifier, visible)) {
        [self createNSError];
    }
}

- (void)setDrawOrder:(NSInteger)drawOrder
{
    _drawOrder = drawOrder;

    if (!tangramInstance->markerSetDrawOrder(self.identifier, (int)drawOrder)) {
        [self createNSError];
    }
}

- (void)setIcon:(UIImage *)icon
{
    _icon = icon;

    CGImage* cgImage = [icon CGImage];
    size_t w = CGImageGetHeight(cgImage);
    size_t h = CGImageGetWidth(cgImage);
    std::vector<unsigned int> bitmap;

    bitmap.resize(w * h);

    CGColorSpaceRef colorSpace = CGImageGetColorSpace(cgImage);
    // iOS only supports contexts with pre-multiplied alpha, so we transform it below.
    CGContextRef cgContext = CGBitmapContextCreate(bitmap.data(), w, h, 8, w * 4,
        colorSpace, kCGImageAlphaPremultipliedLast);

    // Flip image upside down-horizontally
    CGAffineTransform flipAffineTransform = CGAffineTransformMake(1, 0, 0, -1, 0, h);
    CGContextConcatCTM(cgContext, flipAffineTransform);

    CGContextDrawImage(cgContext, CGRectMake(0, 0, w, h), cgImage);
    CGContextRelease(cgContext);

    // For each pixel in the image, convert from BGRA to RGBA and if A != 0 then un-pre-multiply alpha.
    // TODO: This is wasteful! Instead we could ingest pre-multiplied data with a flag or enum and
    // alter the rendering mode for this texture appropriately. -MEB 3.30.18
    for (auto& pixel : bitmap) {
        auto* p = reinterpret_cast<unsigned char*>(&pixel);
        unsigned int a = p[3];
        unsigned int b = p[0];
        if (a == 0) {
            p[0] = p[2];
            p[2] = b;
        } else {
            p[0] = p[2] * 255 / a;
            p[1] = p[1] * 255 / a;
            p[2] = b * 255 / a;
        }
    }

    if (!tangramInstance->markerSetBitmap(self.identifier, static_cast<int>(w), static_cast<int>(h), bitmap.data())) {
        [self createNSError];
    }
}

- (BOOL)getError:(NSError**)error
{
    if (self->error) {
        if (error) {
            *error = self->error;
        }

        self->error = nil;
        return YES;
    }

    return NO;
}

@end

