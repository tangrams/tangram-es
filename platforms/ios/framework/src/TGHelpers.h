//
//  TGHelpers.h
//  TangramMap
//
//  Created by Karim Naaji on 10/18/16.
//  Updated by Matt Blair on 7/13/18.
//  Copyright (c) 2017 Mapzen. All rights reserved.
//

#import "TGTypes.h"
#import <Foundation/Foundation.h>

#include "map.h"

@interface TGHelpers : NSObject

+ (Tangram::EaseType)convertEaseTypeFrom:(TGEaseType)ease;
+ (TGError)convertTGErrorTypeFrom:(Tangram::Error)error;
+ (NSError *)errorFromSceneError:(Tangram::SceneError)updateError;

@end


