//
//  TGMarkerPickResult.mm
//  TangramMap
//
//  Created by Karim Naaji on 01/03/17.
//  Copyright (c) 2017 Mapzen. All rights reserved.
//

#import "TGMarkerPickResult.h"

@interface TGMarkerPickResult ()

@property (assign, nonatomic) TGGeoPoint coordinates;
@property (assign, nonatomic) TGMapMarkerId identifier;

@end

@implementation TGMarkerPickResult

- (instancetype) initWithCoordinates:(TGGeoPoint)coordinates identifier:(TGMapMarkerId)identifier
{
    self = [super init];

    if (self) {
        self.coordinates = coordinates;
        self.identifier = identifier;
    }

    return self;
}

@end

