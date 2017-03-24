//
//  TGMarker.mm
//  TangramMap
//
//  Created by Karim Naaji on 2/17/17.
//  Copyright (c) 2017 Mapzen. All rights reserved.
//

#import "TGMarker.h"
#import "TGMapViewController.h"
#import "TGMapViewController+Internal.h"
#import "TGHelpers.h"
#import "tangram.h"

enum class TGMarkerType {
    point,
    polygon,
    polyline,
    none,
};

@interface TGMarker () {
    Tangram::Map* tangramInstance;
    Tangram::MarkerID identifier;
    TGMarkerType type;
}

@property (copy, nonatomic) NSString* stylingString;
@property (copy, nonatomic) NSString* stylingPath;
@property (assign, nonatomic) TGGeoPoint point;
@property (strong, nonatomic) TGGeoPolyline* polyline;
@property (strong, nonatomic) TGGeoPolygon* polygon;
@property (assign, nonatomic) BOOL visible;
@property (assign, nonatomic) NSInteger drawOrder;
@property (strong, nonatomic) UIImage* icon;
@property (weak, nonatomic) TGMapViewController* map;

- (NSError *)createNSError;

@end

@implementation TGMarker

- (instancetype)init
{
    self = [super init];

    if (self) {
        type = TGMarkerType::none;
        self.visible = YES;
        self.drawOrder = 0;
    }

    return self;
}

- (instancetype)initWithMapView:(TGMapViewController *)mapView
{
    self = [super init];

    if (self) {
        type = TGMarkerType::none;
        self.visible = YES;
        self.drawOrder = 0;
        [self map:mapView];
    }

    return self;
}

- (NSError *)createNSError
{
    NSMutableDictionary* userInfo = [[NSMutableDictionary alloc] init];
    // TODO: add enum for object error
    // [userInfo setObject: forKey:@"TGMarker"];

    return [NSError errorWithDomain:@"TGMarker"
                               code:0
                           userInfo:userInfo];
}

- (NSError *)stylingString:(NSString *)styling
{
    _stylingString = styling;
    _stylingPath = nil;

    if (!tangramInstance || !identifier) { return nil; }

     if (!tangramInstance->markerSetStylingFromString(identifier, [styling UTF8String])) {
        return [self createNSError];
    }

    return nil;
}

- (NSError *)stylingPath:(NSString *)path
{
    _stylingPath = path;
    _stylingString = nil;

    if (!tangramInstance || !identifier) { return nil; }

    if (!tangramInstance->markerSetStylingFromPath(identifier, [path UTF8String])) {
        return [self createNSError];
    }

    return nil;
}

- (NSError *)point:(TGGeoPoint)coordinates
{
    _point = coordinates;
    type = TGMarkerType::point;

    Tangram::LngLat lngLat(coordinates.longitude, coordinates.latitude);

    if (!tangramInstance || !identifier) { return nil; }

    if (!tangramInstance->markerSetPoint(identifier, lngLat)) {
        return [self createNSError];
    }

    return nil;
}

- (NSError *)pointEased:(TGGeoPoint)coordinates seconds:(float)seconds easeType:(TGEaseType)ease
{
    _point = coordinates;
    type = TGMarkerType::point;

    if (!tangramInstance || !identifier) { return nil; }

    Tangram::LngLat lngLat(coordinates.longitude, coordinates.latitude);

    if (!tangramInstance->markerSetPointEased(identifier, lngLat, seconds, [TGHelpers convertEaseTypeFrom:ease])) {
        return [self createNSError];
    }

    return nil;
}

- (NSError *)polyline:(TGGeoPolyline *)polyline
{
    _polyline = polyline;
    type = TGMarkerType::polyline;

    if (polyline.count < 2 || !tangramInstance || !identifier) {
        return nil;
    }

    auto polylineCoords = reinterpret_cast<Tangram::LngLat*>([polyline coordinates]);

    if (!tangramInstance->markerSetPolyline(identifier, polylineCoords, polyline.count)) {
        return [self createNSError];
    }

    return nil;
}

- (NSError *)polygon:(TGGeoPolygon *)polygon
{
    _polygon = polygon;
    type = TGMarkerType::polygon;

    if (polygon.count < 3 || !tangramInstance || !identifier) { return nil; }

    auto polygonCoords = reinterpret_cast<Tangram::LngLat*>([polygon coordinates]);

    if (!tangramInstance->markerSetPolygon(identifier, polygonCoords, [polygon rings], [polygon ringsCount])) {
        return [self createNSError];
    }

    return nil;
}

- (NSError *)visible:(BOOL)visible
{
    _visible = visible;

    if (!tangramInstance || !identifier) { return nil; }

    if (!tangramInstance->markerSetVisible(identifier, visible)) {
        return [self createNSError];
    }

    return nil;
}

- (NSError *)drawOrder:(NSInteger)drawOrder
{
    _drawOrder = drawOrder;

    if (!tangramInstance || !identifier) { return nil; }

     if (!tangramInstance->markerSetDrawOrder(identifier, (int)drawOrder)) {
        return [self createNSError];
    }

    return nil;
}

- (NSError *)icon:(UIImage *)icon
{
    _icon = icon;

    if (!tangramInstance || !identifier) { return nil; }

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

    if (!tangramInstance->markerSetBitmap(identifier, w, h, bitmap.data())) {
        return [self createNSError];
    }

    return nil;
}

- (NSError *)map:(TGMapViewController *)mapView
{
    // remove marker from current view
    if (!mapView && tangramInstance && identifier) {
        if (!tangramInstance->markerRemove(identifier)) {
            return [self createNSError];
        }

        tangramInstance = nullptr;
        _map = nil;
        return nil;
    }

    if (![mapView map] || [mapView map] == tangramInstance) { return nil; }

    // Removes the marker from the previous map view
    if (tangramInstance && _map) {
        if (!tangramInstance->markerRemove(identifier)) {
            return [self createNSError];
        }

        [_map removeMarker:identifier];
    }

    tangramInstance = [mapView map];
    _map = mapView;

    // Create a new marker identifier for this view
    identifier = tangramInstance->markerAdd();

    if (!identifier) { return [self createNSError]; }

    [_map addMarker:self withIdentifier:identifier];

    // Set the geometry type
    switch (type) {
        case TGMarkerType::point: {
            [self point:self.point];

            if (self.icon) {
                [self icon:self.icon];
            }
        }
        break;
        case TGMarkerType::polygon: {
            [self polygon:self.polygon];
        }
        break;
        case TGMarkerType::polyline: {
            [self polyline:self.polyline];
        }
        case TGMarkerType::none:
        break;
    }

    // Update styling
    if (self.stylingString) {
        if (!tangramInstance->markerSetStylingFromString(identifier, [self.stylingString UTF8String])) {
            return [self createNSError];
        }
    } else if (self.stylingPath) {
        if (!tangramInstance->markerSetStylingFromPath(identifier, [self.stylingPath UTF8String])) {
            return [self createNSError];
        }
    }

    if (!tangramInstance->markerSetVisible(identifier, self.visible)) {
        return [self createNSError];
    }

    if (!tangramInstance->markerSetDrawOrder(identifier, (int)self.drawOrder)) {
        return [self createNSError];
    }

    return nil;
}

@end

