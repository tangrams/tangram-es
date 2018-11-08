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

@property (assign, nonatomic) CLLocationCoordinate2D coordinate;
@property (assign, nonatomic) TGLabelType type;
@property (strong, nonatomic) TGFeatureProperties* properties;

@end

@implementation TGLabelPickResult

- (instancetype) initWithCoordinate:(CLLocationCoordinate2D)coordinate type:(TGLabelType)type properties:(TGFeatureProperties *)properties
{
    self = [super init];

    if (self) {
        self.coordinate = coordinate;
        self.type = type;
        self.properties = properties;
    }

    return self;
}

@end

