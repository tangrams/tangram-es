//
//  TGMarkerPickResult.mm
//  TangramMap
//
//  Created by Karim Naaji on 01/03/17.
//  Updated by Matt Blair on 8/21/18.
//  Copyright (c) 2017 Mapzen. All rights reserved.
//

#import "TGMarker.h"
#import "TGMarkerPickResult.h"
#import "TGMarkerPickResult+Internal.h"

@interface TGMarkerPickResult ()

@property (assign, nonatomic) CLLocationCoordinate2D coordinate;
@property (strong, nonatomic) TGMarker* marker;

@end

@implementation TGMarkerPickResult

- (instancetype) initWithCoordinate:(CLLocationCoordinate2D)coordinate marker:(TGMarker *)marker
{
    self = [super init];

    if (self) {
        self.coordinate = coordinate;
        self.marker = marker;
    }

    return self;
}

@end

