//
//  TGGeoPolygon.mm
//  TangramMap
//
//  Created by Karim Naaji on 10/27/16.
//  Updated by Matt Blair on 8/21/18.
//  Copyright (c) 2017 Mapzen. All rights reserved.
//

#import "TGGeoPolygon.h"

@implementation TGGeoPolygon

- (instancetype)initWithRings:(NSArray<TGGeoPolyline *> *)rings
{
    self = [super init];

    if (self) {
        _rings = rings;
    }

    return self;
}

@end
