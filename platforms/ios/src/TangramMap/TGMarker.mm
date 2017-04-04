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

- (void)createNSError:(NSError **)error;

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

        [self map:mapView error:nil];
    }

    return self;
}

- (void)createNSError:(NSError **)error
{
    if (!error) {
        return;
    }

    NSMutableDictionary* userInfo = [[NSMutableDictionary alloc] init];

    // TODO: add enum for object error
    // [userInfo setObject:enumErrorType forKey:@"TGMarker"];

    *error = [NSError errorWithDomain:@"TGMarker"
                                 code:0
                             userInfo:userInfo];
}

- (BOOL)stylingString:(NSString *)styling error:(NSError **)error
{
    _stylingString = styling;
    _stylingPath = nil;

    if (!tangramInstance || !identifier) {
        [self createNSError:error];
        return NO;
    }

     if (!tangramInstance->markerSetStylingFromString(identifier, [styling UTF8String])) {
        [self createNSError:error];
        return NO;
    }

    return YES;
}

- (BOOL)stylingPath:(NSString *)path error:(NSError **)error
{
    _stylingPath = path;
    _stylingString = nil;

    if (!tangramInstance || !identifier) {
        [self createNSError:error];
        return NO;
    }

    if (!tangramInstance->markerSetStylingFromPath(identifier, [path UTF8String])) {
        [self createNSError:error];
        return NO;
    }

    return YES;
}

- (BOOL)point:(TGGeoPoint)coordinates error:(NSError **)error
{
    _point = coordinates;
    type = TGMarkerType::point;

    Tangram::LngLat lngLat(coordinates.longitude, coordinates.latitude);

    if (!tangramInstance || !identifier) {
        [self createNSError:error];
        return NO;
    }

    if (!tangramInstance->markerSetPoint(identifier, lngLat)) {
        [self createNSError:error];
        return NO;
    }

    return YES;
}

- (BOOL)pointEased:(TGGeoPoint)coordinates seconds:(float)seconds easeType:(TGEaseType)ease error:(NSError **)error
{
    _point = coordinates;
    type = TGMarkerType::point;

    if (!tangramInstance || !identifier) {
        [self createNSError:error];
        return NO;
    }

    Tangram::LngLat lngLat(coordinates.longitude, coordinates.latitude);

    if (!tangramInstance->markerSetPointEased(identifier, lngLat, seconds, [TGHelpers convertEaseTypeFrom:ease])) {
        [self createNSError:error];
        return NO;
    }

    return YES;
}

- (BOOL)polyline:(TGGeoPolyline *)polyline error:(NSError **)error
{
    _polyline = polyline;
    type = TGMarkerType::polyline;

    if (polyline.count < 2 || !tangramInstance || !identifier) {
        [self createNSError:error];
        return NO;
    }

    auto polylineCoords = reinterpret_cast<Tangram::LngLat*>([polyline coordinates]);

    if (!tangramInstance->markerSetPolyline(identifier, polylineCoords, polyline.count)) {
        [self createNSError:error];
        return NO;
    }

    return YES;
}

- (BOOL)polygon:(TGGeoPolygon *)polygon error:(NSError **)error
{
    _polygon = polygon;
    type = TGMarkerType::polygon;

    if (polygon.count < 3 || !tangramInstance || !identifier) {
        [self createNSError:error];
        return NO;
    }

    auto polygonCoords = reinterpret_cast<Tangram::LngLat*>([polygon coordinates]);

    if (!tangramInstance->markerSetPolygon(identifier, polygonCoords, [polygon rings], [polygon ringsCount])) {
        [self createNSError:error];
        return NO;
    }

    return YES;
}

- (BOOL)visible:(BOOL)visible error:(NSError **)error
{
    _visible = visible;

    if (!tangramInstance || !identifier) {
        [self createNSError:error];
        return NO;
    }

    if (!tangramInstance->markerSetVisible(identifier, visible)) {
        [self createNSError:error];
        return NO;
    }

    return YES;
}

- (BOOL)drawOrder:(NSInteger)drawOrder error:(NSError **)error
{
    _drawOrder = drawOrder;

    if (!tangramInstance || !identifier) {
        [self createNSError:error];
        return NO;
    }

     if (!tangramInstance->markerSetDrawOrder(identifier, (int)drawOrder)) {
        [self createNSError:error];
        return NO;
    }

    return YES;
}

- (BOOL)icon:(UIImage *)icon error:(NSError **)error
{
    _icon = icon;

    if (!tangramInstance || !identifier) {
        [self createNSError:error];
        return NO;
    }

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
        [self createNSError:error];
        return NO;
    }

    return YES;
}

- (BOOL)map:(nullable TGMapViewController *)mapView error:(NSError **)error
{
    // remove marker from current view
    if (!mapView && tangramInstance && identifier) {
        if (!tangramInstance->markerRemove(identifier)) {
            [self createNSError:error];
            return NO;
        }

        tangramInstance = nullptr;
        _map = nil;
        return YES;
    }

    if (![mapView map] || [mapView map] == tangramInstance) {
        [self createNSError:error];
        return NO;
    }

    // Removes the marker from the previous map view
    if (tangramInstance && _map) {
        if (!tangramInstance->markerRemove(identifier)) {
            [self createNSError:error];
            return NO;
        }

        [_map removeMarker:identifier];
    }

    tangramInstance = [mapView map];
    _map = mapView;

    // Create a new marker identifier for this view
    identifier = tangramInstance->markerAdd();

    if (!identifier) {
        [self createNSError:error];
        return NO;
    }

    [_map addMarker:self withIdentifier:identifier];

    // Set the geometry type
    switch (type) {
        case TGMarkerType::point: {
            if (![self point:self.point error:error]) { return NO; }

            if (self.icon) {
                if (![self icon:self.icon error:error]) { return NO; }
            }
        }
        break;
        case TGMarkerType::polygon: {
            if (![self polygon:self.polygon error:error]) { return NO; }
        }
        break;
        case TGMarkerType::polyline: {
            if (![self polyline:self.polyline error:error]) { return NO; }
        }
        case TGMarkerType::none:
        break;
    }

    // Update styling
    if (self.stylingString) {
        if (!tangramInstance->markerSetStylingFromString(identifier, [self.stylingString UTF8String])) {
            [self createNSError:error];
            return NO;
        }
    } else if (self.stylingPath) {
        if (!tangramInstance->markerSetStylingFromPath(identifier, [self.stylingPath UTF8String])) {
            [self createNSError:error];
            return NO;
        }
    }

    if (!tangramInstance->markerSetVisible(identifier, self.visible)) {
        [self createNSError:error];
        return NO;
    }

    if (!tangramInstance->markerSetDrawOrder(identifier, (int)self.drawOrder)) {
        [self createNSError:error];
        return NO;
    }

    return YES;
}

@end

