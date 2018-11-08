//
//  TGGeoPolyline.mm
//  TangramMap
//
//  Created by Karim Naaji on 10/27/16.
//  Updated by Matt Blair on 8/21/18.
//  Copyright (c) 2017 Mapzen. All rights reserved.
//

#import "TGGeoPolyline.h"

#include <vector>

@interface TGGeoPolyline () {
    std::vector<CLLocationCoordinate2D> coordinates;
}

@end

@implementation TGGeoPolyline

- (instancetype)initWithSize:(NSUInteger)size
{
    self = [super init];

    if (self) {
        coordinates.reserve(size);
    }

    return self;
}

- (void)addPoint:(CLLocationCoordinate2D)point
{
    coordinates.push_back(point);
}

- (NSUInteger)count
{
    return coordinates.size();
}

- (CLLocationCoordinate2D *)coordinates
{
    return coordinates.data();
}

- (void)removeAll
{
    coordinates.clear();
}

@end

