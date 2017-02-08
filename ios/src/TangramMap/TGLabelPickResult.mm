//
//  TGLabelPickResult.mm
//  tangram
//
//  Created by Karim Naaji on 11/02/16.
//

#import "TGLabelPickResult.h"

@interface TGLabelPickResult ()

@property (assign, nonatomic) TGGeoPoint coordinates;
@property (assign, nonatomic) TGLabelType type;
@property (strong, nonatomic) NSDictionary* properties;

@end

@implementation TGLabelPickResult

- (instancetype) initWithCoordinates:(TGGeoPoint)coordinates type:(TGLabelType)type properties:(NSDictionary *)properties
{
    self = [super init];

    if (self) {
        self.coordinates = coordinates;
        self.type = type;
        self.properties = properties;
    }

    return self;
}

@end

