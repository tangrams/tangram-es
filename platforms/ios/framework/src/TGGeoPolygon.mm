//
//  TGGeoPolygon.mm
//  TangramMap
//
//  Created by Karim Naaji on 10/27/16.
//  Updated by Matt Blair on 8/21/18.
//  Copyright (c) 2017 Mapzen. All rights reserved.
//

#import "TGGeoPolygon.h"

#include <vector>

@interface TGGeoPolygon () {
    std::vector<CLLocationCoordinate2D> coordinates;
    std::vector<int> rings;
}

@end

@implementation TGGeoPolygon

- (instancetype)initWithSize:(NSUInteger)size
{
    self = [super init];

    if (self) {
        coordinates.reserve(size);
    }

    return self;
}

- (void)startPath:(CLLocationCoordinate2D)point withSize:(NSUInteger)size
{
    coordinates.reserve(coordinates.size() + size);
    coordinates.push_back(point);

    rings.emplace_back(1);
}

- (void)startPath:(CLLocationCoordinate2D)point
{
    [self startPath:point withSize:0];
}

- (void)addPoint:(CLLocationCoordinate2D)point
{
    if (rings.size() == 0) { return; }

    coordinates.push_back(point);
    rings.back()++;
}

- (NSUInteger)count
{
    return coordinates.size();
}

- (NSUInteger)ringsCount
{
    return rings.size();
}

- (CLLocationCoordinate2D *)coordinates
{
    return coordinates.data();
}

- (int *)rings
{
    return rings.data();
}

- (void)removeAll
{
    coordinates.clear();
    rings.clear();
}

@end
