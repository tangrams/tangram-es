//
//  TGMarker.mm
//  TangramMap
//
//  Created by Karim Naaji on 2/17/17.
//  Copyright (c) 2017 Mapzen. All rights reserved.
//

#import "TGMarker.h"
#import "TGMarker+Internal.h"
#import "TGMapViewController.h"
#import "TGMapViewController+Internal.h"
#import "TGHelpers.h"
#import "TGTypes.h"
#import "map.h"

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

- (void)pointEased:(TGGeoPoint)coordinates seconds:(float)seconds easeType:(TGEaseType)ease
{
    [self clearGeometry];
    _point = coordinates;

    Tangram::LngLat lngLat(coordinates.longitude, coordinates.latitude);

    if (!tangramInstance->markerSetPointEased(self.identifier, lngLat, seconds, [TGHelpers convertEaseTypeFrom:ease])) {
        [self createNSError];
    }
}

- (void)setPoint:(TGGeoPoint)coordinates
{
    [self clearGeometry];
    _point = coordinates;

    Tangram::LngLat lngLat(coordinates.longitude, coordinates.latitude);

    if (!tangramInstance->markerSetPoint(self.identifier, lngLat)) {
        [self createNSError];
    }
}

- (void)setPolyline:(TGGeoPolyline *)polyline
{
    [self clearGeometry];
    _polyline = polyline;

    auto polylineCoords = reinterpret_cast<Tangram::LngLat*>([polyline coordinates]);

    if (!tangramInstance->markerSetPolyline(self.identifier, polylineCoords, polyline.count)) {
        [self createNSError];
    }
}

- (void)setPolygon:(TGGeoPolygon *)polygon
{
    [self clearGeometry];
    _polygon = polygon;

    auto polygonCoords = reinterpret_cast<Tangram::LngLat*>([polygon coordinates]);

    if (!tangramInstance->markerSetPolygon(self.identifier, polygonCoords, [polygon rings], [polygon ringsCount])) {
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
    CGContextRef cgContext = CGBitmapContextCreate(bitmap.data(), w, h, 8, w * 4,
        colorSpace, kCGImageAlphaPremultipliedLast);

    // Flip image upside down-horizontally
    CGAffineTransform flipAffineTransform = CGAffineTransformMake(1, 0, 0, -1, 0, h);
    CGContextConcatCTM(cgContext, flipAffineTransform);

    CGContextDrawImage(cgContext, CGRectMake(0, 0, w, h), cgImage);
    CGContextRelease(cgContext);

    if (!tangramInstance->markerSetBitmap(self.identifier, w, h, bitmap.data())) {
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

