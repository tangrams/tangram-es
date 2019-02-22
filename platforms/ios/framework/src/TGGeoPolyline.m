//
//  TGGeoPolyline.mm
//  TangramMap
//
//  Created by Karim Naaji on 10/27/16.
//  Updated by Matt Blair on 8/21/18.
//  Copyright (c) 2017 Mapzen. All rights reserved.
//

#import "TGGeoPolyline.h"
#import "TGGeoPoints+Internal.h"

@implementation TGGeoPolyline

- (instancetype)initWithCoordinates:(const CLLocationCoordinate2D *)coordinates count:(NSUInteger)count
{
    self = [super initWithCoordinates:coordinates count:count];

    return self;
}

@end

