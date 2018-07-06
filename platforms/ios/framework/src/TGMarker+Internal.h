//
//  TGMarker+Internal.h
//  TangramMap
//
//  Created by Karim Naaji on 4/10/17.
//  Copyright (c) 2017 Mapzen. All rights reserved.
//

#import "map.h"

@interface TGMarker ()

NS_ASSUME_NONNULL_BEGIN

- (instancetype)initWithMap:(Tangram::Map*)map;

NS_ASSUME_NONNULL_END

@property (assign, nonatomic) Tangram::MarkerID identifier;

@end
