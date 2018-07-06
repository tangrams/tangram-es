//
//  TGMarkerPickResult.mm
//  TangramMap
//
//  Created by Karim Naaji on 01/03/17.
//  Copyright (c) 2017 Mapzen. All rights reserved.
//

#import "TGMarkerPickResult.h"
#import "TGMarkerPickResult+Internal.h"

@interface TGMarkerPickResult ()

@property (assign, nonatomic) TGGeoPoint coordinates;
@property (strong, nonatomic) TGMarker* marker;

@end

@implementation TGMarkerPickResult

- (instancetype) initWithCoordinates:(TGGeoPoint)coordinates marker:(TGMarker *)marker
{
    self = [super init];

    if (self) {
        self.coordinates = coordinates;
        self.marker = marker;
    }

    return self;
}

@end

