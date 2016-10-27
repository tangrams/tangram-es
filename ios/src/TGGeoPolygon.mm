//
//  TGGeoPolygon.mm
//  tangram
//
//  Created by Karim Naaji on 10/27/16.
//
//

#import "TGGeoPolygon.h"

#include <vector>

using Ring = std::vector<TGGeoPoint>;

@interface TGGeoPolygon () {
    std::vector<Ring> polygons;
    unsigned int points;
}

@end

@implementation TGGeoPolygon

- (id)init
{
    self = [super init];

    if (self) {
        points = 0;
    }

    return self;
}

- (void)startPath:(TGGeoPoint)latlon withSize:(unsigned int)size
{
    polygons.emplace_back();
    polygons.back().reserve(size);
    polygons.back().push_back(latlon);

    points++;
}

- (void)startPath:(TGGeoPoint)latlon
{
    [self startPath:latlon withSize:0];
}

- (void)addPoint:(TGGeoPoint)latlon
{
    if (polygons.size() == 0) { return; }
    polygons.back().push_back(latlon);

    points++;
}

- (unsigned int)count
{
    return points;
}

- (void*)data
{
    return (void*)&polygons;
}

- (void)removeAll
{
    polygons.clear();
}

@end
