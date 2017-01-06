//
//  TGMarkerPickResult.mm
//  tangram
//
//  Created by Karim Naaji on 01/03/17.
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

