//
//  TGGeoPoints.m
//  TangramMap
//
//  Created by Matt Blair on 2/21/19.
//  Copyright (c) 2019 Mapzen. All rights reserved.
//

#import "TGGeoPoints+Internal.h"

@implementation TGGeoPoints

- (instancetype)initWithCoordinates:(const CLLocationCoordinate2D *)coordinates count:(NSUInteger)count
{
    self = [super init];

    if (self) {
        size_t size = sizeof(CLLocationCoordinate2D) * count;
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

