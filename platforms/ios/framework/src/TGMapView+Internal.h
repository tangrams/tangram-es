//
//  TGMapView+Internal.h
//  TangramMap
//
//  Created by Matt Blair on 7/10/18.
//

#import "TGMapView.h"

#include "map.h"
#include <vector>
#include <memory>

@interface TGMapView ()

NS_ASSUME_NONNULL_BEGIN

- (BOOL)removeDataSource:(std::shared_ptr<Tangram::TileSource>)tileSource name:(NSString *)name;

- (void)clearDataSource:(std::shared_ptr<Tangram::TileSource>)tileSource;

- (Tangram::SceneReadyCallback)sceneReadyListener;

NS_ASSUME_NONNULL_END

@property (assign, nonatomic, nullable) Tangram::Map* map;
@property (strong, nonatomic, nullable) id<TGURLHandler> urlHandler;

@end
