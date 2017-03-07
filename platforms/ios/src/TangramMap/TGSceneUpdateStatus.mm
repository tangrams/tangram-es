//
//  TGSceneUpdateStatus.h
//  TangramMap
//
//  Created by Karim Naaji on 3/07/17.
//  Copyright (c) 2017 Mapzen. All rights reserved.
//


#import "TGSceneUpdateStatus.h"
#import "TGSceneUpdate.h"
#import "TGHelpers.h"

@interface TGSceneUpdateStatus ()

@property (assign, nonatomic) TGSceneUpdate* sceneUpdate;
@property (assign, nonatomic) TGSceneUpdateError error;

@end

@implementation TGSceneUpdateStatus

- (instancetype)initWithSceneUpdate:(TGSceneUpdate *)sceneUpdate sceneUpdateError:(TGSceneUpdateError)error
{
    self = [super init];

    if (self) {
        self.sceneUpdate = sceneUpdate;
        self.error = error;
    }

    return self;
}

+ (NSArray<TGSceneUpdateStatus *> *)sceneUpdateStatuses:(const std::vector<Tangram::SceneUpdateStatus>&)updateStatuses
{
    NSMutableArray* nsUpdateStatuses = [[NSMutableArray alloc] initWithCapacity:(NSUInteger)updateStatuses.size()];

    for (const Tangram::SceneUpdateStatus& updateStatus : updateStatuses) {
        NSString* path = [NSString stringWithUTF8String:updateStatus.update.path.c_str()];
        NSString* value = [NSString stringWithUTF8String:updateStatus.update.value.c_str()];
        TGSceneUpdate* udpate = [[TGSceneUpdate alloc] initWithPath:path value:value];
        TGSceneUpdateError error = [TGHelpers convertSceneUpdateErrorTypeFrom:updateStatus.error];

        TGSceneUpdateStatus* tgUpdateStatus = [[TGSceneUpdateStatus alloc] initWithSceneUpdate:udpate
                                                                              sceneUpdateError:error];

        [nsUpdateStatuses addObject:tgUpdateStatus];
    }

    return nsUpdateStatuses;
}

@end

