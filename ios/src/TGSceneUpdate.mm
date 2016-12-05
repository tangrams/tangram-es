//
//  TGSceneUpdate.mm
//  tangram
//
//  Created by Karim Naaji on 11/05/16.
//
//

#import "TGSceneUpdate.h"

@implementation TGSceneUpdate

- (id)initWithPath:(NSString *)path value:(NSString *)value;
{
    self = [super init];

    if (self) {
        self.path = path;
        self.value = value;
    }

    return self;
}

@end
