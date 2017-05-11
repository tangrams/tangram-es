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
#import "tangram.h"

@interface TGMarker () {
    Tangram::Map* tangramInstance;
}

@property (copy, nonatomic) NSString* stylingString;
@property (copy, nonatomic) NSString* stylingPath;
@property (assign, nonatomic) TGGeoPoint point;
@property (strong, nonatomic) TGGeoPolyline* polyline;
@property (strong, nonatomic) TGGeoPolygon* polygon;
@property (assign, nonatomic) BOOL visible;
@property (assign, nonatomic) NSInteger drawOrder;
@property (strong, nonatomic) UIImage* icon;

- (void)createNSError:(NSError **)error;
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
    self.polyline = nil;
    self.polygon = nil;
    self.point = {NAN, NAN};
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

    if (!tangramInstance->markerSetStylingFromString(self.identifier, [styling UTF8String])) {
        [self createNSError:error];
        return NO;
    }

    return YES;
}

- (BOOL)stylingPath:(NSString *)path error:(NSError **)error
{
    _stylingPath = path;
    _stylingString = nil;

    if (!tangramInstance->markerSetStylingFromPath(self.identifier, [path UTF8String])) {
        [self createNSError:error];
        return NO;
    }

    return YES;
}

- (BOOL)pointEased:(TGGeoPoint)coordinates seconds:(float)seconds easeType:(TGEaseType)ease error:(NSError **)error
{
    [self clearGeometry];
    _point = coordinates;

    Tangram::LngLat lngLat(coordinates.longitude, coordinates.latitude);

    if (!tangramInstance->markerSetPointEased(self.identifier, lngLat, seconds, [TGHelpers convertEaseTypeFrom:ease])) {
        [self createNSError:error];
        return NO;
    }

    return YES;
}

- (BOOL)point:(TGGeoPoint)coordinates error:(NSError **)error
{
    [self clearGeometry];
    _point = coordinates;

    Tangram::LngLat lngLat(coordinates.longitude, coordinates.latitude);

    if (!tangramInstance->markerSetPoint(self.identifier, lngLat)) {
        [self createNSError:error];
        return NO;
    }

    return YES;
}

- (BOOL)polyline:(TGGeoPolyline *)polyline error:(NSError **)error
{
    [self clearGeometry];
    _polyline = polyline;

    auto polylineCoords = reinterpret_cast<Tangram::LngLat*>([polyline coordinates]);

    if (!tangramInstance->markerSetPolyline(self.identifier, polylineCoords, polyline.count)) {
        [self createNSError:error];
        return NO;
    }

    return YES;
}

- (BOOL)polygon:(TGGeoPolygon *)polygon error:(NSError **)error
{
    [self clearGeometry];
    _polygon = polygon;

    auto polygonCoords = reinterpret_cast<Tangram::LngLat*>([polygon coordinates]);

    if (!tangramInstance->markerSetPolygon(self.identifier, polygonCoords, [polygon rings], [polygon ringsCount])) {
        [self createNSError:error];
        return NO;
    }
}

- (BOOL)visible:(BOOL)visible error:(NSError **)error
{
    _visible = visible;

    if (!tangramInstance->markerSetVisible(self.identifier, visible)) {
        [self createNSError:error];
        return NO;
    }

    return YES;
}

- (BOOL)drawOrder:(NSInteger)drawOrder error:(NSError **)error
{
    _drawOrder = drawOrder;

    if (!tangramInstance->markerSetDrawOrder(self.identifier, (int)drawOrder)) {
        [self createNSError:error];
        return NO;
    }

    return YES;
}

- (BOOL)icon:(UIImage *)icon error:(NSError **)error
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
        [self createNSError:error];
        return NO;
    }

    return YES;
}

@end

