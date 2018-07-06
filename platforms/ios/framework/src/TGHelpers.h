//
//  TGHelpers.h
//  TangramMap
//
//  Created by Karim Naaji on 10/18/16.
//  Copyright (c) 2017 Mapzen. All rights reserved.
//

#import "TGMapViewController.h"
#import "map.h"

@interface TGHelpers : NSObject

+ (Tangram::EaseType)convertEaseTypeFrom:(TGEaseType)ease;
+ (TGError)convertTGErrorTypeFrom:(Tangram::Error)error;
+ (NSError *)errorFromSceneError:(Tangram::SceneError)updateError;

@end


