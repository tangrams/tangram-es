//
//  TGLabelPickResult.mm
//  TangramMap
//
//  Created by Karim Naaji on 11/02/16.
//  Copyright (c) 2017 Mapzen. All rights reserved.
//

#import "TGLabelPickResult.h"
#import "TGMarkerPickResult+Internal.h"

@interface TGLabelPickResult ()

@property (assign, nonatomic) TGGeoPoint coordinates;
@property (assign, nonatomic) TGLabelType type;
@property (strong, nonatomic) TGFeatureProperties* properties;

@end

@implementation TGLabelPickResult

- (instancetype) initWithCoordinates:(TGGeoPoint)coordinates type:(TGLabelType)type properties:(TGFeatureProperties *)properties
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

