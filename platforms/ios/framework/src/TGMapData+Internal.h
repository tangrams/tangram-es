//
//  TGMapData+Internal.h
//  TangramMap
//
//  Created by Karim Naaji on 2/24/16.
//  Updated by Matt Blair on 7/13/18.
//  Copyright (c) 2017 Mapzen. All rights reserved.
//

#import "TGMapData.h"
#import <Foundation/Foundation.h>

#include "data/clientDataSource.h"
#include <memory>

@interface TGMapData ()

NS_ASSUME_NONNULL_BEGIN

- (instancetype)initWithMapView:(__weak TGMapView *)mapView name:(NSString *)name source:(std::shared_ptr<Tangram::ClientDataSource>)source;

NS_ASSUME_NONNULL_END

@end
