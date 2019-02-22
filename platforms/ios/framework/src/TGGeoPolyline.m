//
//  TGGeoPolyline.mm
//  TangramMap
//
//  Created by Karim Naaji on 10/27/16.
//  Updated by Matt Blair on 8/21/18.
//  Copyright (c) 2017 Mapzen. All rights reserved.
//

#import "TGGeoPolyline.h"

@implementation TGGeoPolyline

- (instancetype)initWithCoordinates:(const CLLocationCoordinate2D *)coordinates count:(NSUInteger)count
{
    self = [super init];

    if (self) {
        size_t size = sizeof(*coordinates) * count;
        _coordinates = (CLLocationCoordinate2D *)malloc(size);
        memcpy(_coordinates, coordinates, size);
        _count = count;
    }

    return self;
}

- (void)dealloc
{
    free(_coordinates);
}

@end

