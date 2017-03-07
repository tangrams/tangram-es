//
//  TGSceneUpdateStatus+Internal.h
//  TangramMap
//
//  Created by Karim Naaji on 3/07/17.
//  Copyright (c) 2017 Mapzen. All rights reserved.
//

#import "TGSceneUpdateStatus.h"
#import "tangram.h"

#import <vector>

@interface TGSceneUpdateStatus ()

NS_ASSUME_NONNULL_BEGIN

- (instancetype)initWithSceneUpdate:(TGSceneUpdate *)sceneUpdate sceneUpdateError:(TGSceneUpdateError)error;

+ (NSArray<TGSceneUpdateStatus *> *)sceneUpdateStatuses:(const std::vector<Tangram::SceneUpdateStatus>&)updateStatuses;

NS_ASSUME_NONNULL_END

@end
