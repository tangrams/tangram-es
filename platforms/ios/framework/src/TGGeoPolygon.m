//
//  TGGeoPolygon.mm
//  TangramMap
//
//  Created by Karim Naaji on 10/27/16.
//  Updated by Matt Blair on 8/21/18.
//  Copyright (c) 2017 Mapzen. All rights reserved.
//

#import "TGGeoPolygon.h"
#import "TGGeoPoints+Internal.h"

@implementation TGGeoPolygon

- (instancetype)initWithCoordinates:(CLLocationCoordinate2D *)coordinates count:(NSUInteger)count
{
    self = [super initWithCoordinates:coordinates count:count];

    return self;
}

-(instancetype)initWithCoordinates:(CLLocationCoordinate2D *)coordinates count:(NSUInteger)count interiorPolygons:(NSArray<TGGeoPolygon *> *)interiorPolygons
{
    self = [self initWithCoordinates:coordinates count:count];

    if (self) {
        _interiorPolygons = interiorPolygons;
    }

    return self;
}

@end
