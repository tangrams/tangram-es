//
//  TGMapViewController+Internal.h
//  TangramMap
//
//  Created by Karim Naaji on 2/17/17.
//  Copyright (c) 2017 Mapzen. All rights reserved.
//

#import "TGMapViewController.h"
#import "data/clientGeoJsonSource.h"
#import "map.h"
#import <vector>
#import <memory>

@interface TGMapViewController ()

NS_ASSUME_NONNULL_BEGIN

- (BOOL)removeDataSource:(std::shared_ptr<Tangram::ClientGeoJsonSource>)tileSource name:(NSString *)name;

- (void)clearDataSource:(std::shared_ptr<Tangram::TileSource>)tileSource;

- (Tangram::SceneReadyCallback)sceneReadyListener;

NS_ASSUME_NONNULL_END

@property (assign, nonatomic, nullable) Tangram::Map* map;

@end

