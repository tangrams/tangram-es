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
    std::vector<TGGeoPoint> coordinates;
}

@end

@implementation TGGeoPolyline

- (id)initWithSize:(unsigned int)size
{
    self = [super init];

    if (self) {
        coordinates.reserve(size);
    }

    return self;
}

- (void)addPoint:(TGGeoPoint)latlon
{
    coordinates.push_back(latlon);
}

- (unsigned int)count
{
    return coordinates.size();
}

- (TGGeoPoint*)coordinates
{
    return coordinates.data();
}

- (void)removeAll
{
    coordinates.clear();
}

@end

