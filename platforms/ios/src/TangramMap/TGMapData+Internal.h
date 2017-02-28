//
//  TGMapData+Internal.h
//  TangramMap
//
//  Created by Karim Naaji on 2/24/16.
//  Copyright (c) 2017 Mapzen. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <memory>
#import "data/clientGeoJsonSource.h"

@interface TGMapData ()

NS_ASSUME_NONNULL_BEGIN

- (instancetype)initWithMapView:(TGMapViewController *)mapView name:(NSString *)name source:(std::shared_ptr<Tangram::TileSource>)source;

NS_ASSUME_NONNULL_END

@end
