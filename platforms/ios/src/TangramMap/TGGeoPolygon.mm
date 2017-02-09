//
//  TGGeoPolygon.mm
//  TangramMap
//
//  Created by Karim Naaji on 10/27/16.
//  Copyright (c) 2017 Mapzen. All rights reserved.
//

#import "TGGeoPolygon.h"

#include <vector>

@interface TGGeoPolygon () {
    std::vector<TGGeoPoint> coordinates;
    std::vector<int> rings;
}

@end

@implementation TGGeoPolygon

- (instancetype)initWithSize:(unsigned int)size
{
    self = [super init];

    if (self) {
        coordinates.reserve(size);
    }

    return self;
}

- (void)startPath:(TGGeoPoint)latlon withSize:(unsigned int)size
{
    coordinates.reserve(coordinates.size() + size);
    coordinates.push_back(latlon);

    rings.emplace_back(1);
}

- (void)startPath:(TGGeoPoint)latlon
{
    [self startPath:latlon withSize:0];
}

- (void)addPoint:(TGGeoPoint)latlon
{
    if (rings.size() == 0) { return; }

    coordinates.push_back(latlon);
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

- (TGGeoPoint *)coordinates
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
