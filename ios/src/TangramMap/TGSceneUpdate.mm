//
//  TGSceneUpdate.mm
//  tangram
//
//  Created by Karim Naaji on 11/05/16.
//
//

#import "TGSceneUpdate.h"

@implementation TGSceneUpdate

- (instancetype)initWithPath:(nullable NSString *)path value:(nullable NSString *)value;
{
    self = [super init];

    if (self) {
        self.path = path;
        self.value = value;
    }

    return self;
}

@end
