//
//  TGGeoPolyline.mm
//  tangram
//
//  Created by Karim Naaji on 10/27/16.
//
//

#import "TGGeoPolyline.h"

#include <vector>

@interface TGGeoPolyline () {
    std::vector<TGGeoPoint> points;
}

@end

@implementation TGGeoPolyline

- (id)initWithSize:(unsigned int)size
{
    self = [super init];

    if (self) {
        points.reserve(size);
    }

    return self;
}

- (void)addPoint:(TGGeoPoint)latlon
{
    points.push_back(latlon);
}

- (unsigned int)count
{
    return points.size();
}

- (TGGeoPoint*)data
{
    return points.data();
}

- (void)removeAll
{
    points.clear();
}

@end

