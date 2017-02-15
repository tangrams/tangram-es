//
//  TGSceneUpdate.mm
//  TangramMap
//
//  Created by Karim Naaji on 11/05/16.
//  Copyright (c) 2017 Mapzen. All rights reserved.
//

#import "TGSceneUpdate.h"

@implementation TGSceneUpdate

- (instancetype)initWithPath:(NSString *)path value:(NSString *)value;
{
    self = [super init];

    if (self) {
        self.path = path;
        self.value = value;
    }

    return self;
}

@end
