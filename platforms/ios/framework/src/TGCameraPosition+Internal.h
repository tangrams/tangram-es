//
//  TGCameraPosition+Internal.h
//  tangram
//
//  Created by Matt Blair on 8/9/18.
//

#import "TGCameraPosition.h"

#include "map.h"

@interface TGCameraPosition ()

- (instancetype)initWithCoreCamera:(Tangram::CameraPosition *)camera;

- (void)setToCoreCamera:(Tangram::CameraPosition *)camera;

- (Tangram::CameraPosition)convertToCoreCamera;

@end
