//
//  TGCameraPosition.m
//  TangramMap
//
//  Created by Matt Blair on 7/18/18.
//

#import "TGCameraPosition.h"
#import "TGCameraPosition+Internal.h"
#import <Foundation/Foundation.h>

#include "map.h"

@implementation TGCameraPosition

- (instancetype)initWithCenter:(CLLocationCoordinate2D)center
                          zoom:(CGFloat)zoom
                       bearing:(CLLocationDirection)bearing
                         pitch:(CGFloat)pitch
{
    self = [super init];
    if (self) {
        _center = center;
        _zoom = zoom;
        _bearing = bearing;
        _pitch = pitch;
    }
    return self;
}

- (instancetype)initWithCoreCamera:(Tangram::CameraPosition *)camera
{
    self = [super init];
    if (self) {
        [self setToCoreCamera:camera];
    }
    return self;
}

- (void)setToCoreCamera:(Tangram::CameraPosition *)camera
{
    if (camera == nullptr) {
        return;
    }
    _center.longitude = camera->longitude;
    _center.latitude = camera->latitude;
    _zoom = camera->zoom;
    _bearing = (-180.0 / M_PI) * camera->rotation;
    _pitch = (180.0 / M_PI) * camera->tilt;
}

- (Tangram::CameraPosition)convertToCoreCamera
{
    Tangram::CameraPosition camera;
    camera.longitude = _center.longitude;
    camera.latitude = _center.latitude;
    camera.zoom = _zoom;
    camera.rotation = (-M_PI / 180.0) * _bearing;
    camera.tilt = (M_PI / 180.0) * _pitch;
    return camera;
}

- (id)copyWithZone:(NSZone *)zone
{
    TGCameraPosition *copy = [[TGCameraPosition alloc] init];
    copy.center = _center;
    copy.zoom = _zoom;
    copy.bearing = _bearing;
    copy.pitch = _pitch;
    return copy;
}

- (BOOL)isEqual:(id)other
{
    if (![other isKindOfClass:[self class]]) {
        return NO;
    }
    if (other == self)
    {
        return YES;
    }
    TGCameraPosition *camera = other;
    return (camera.center.latitude == _center.latitude &&
            camera.center.longitude == _center.longitude &&
            camera.zoom == _zoom &&
            camera.bearing == _bearing &&
            camera.pitch == _pitch);
}

@end // implementation TGCameraPosition
