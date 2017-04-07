//
//  TGHelpers.m
//  TangramMap
//
//  Created by Karim Naaji on 10/18/16.
//  Copyright (c) 2017 Mapzen. All rights reserved.
//

#import "TGHelpers.h"
#import "TGTypes.h"

@implementation TGHelpers

+ (Tangram::EaseType)convertEaseTypeFrom:(TGEaseType)ease;
{
    switch (ease) {
        case TGEaseTypeLinear:
            return Tangram::EaseType::linear;
        case TGEaseTypeSine:
            return Tangram::EaseType::sine;
        case TGEaseTypeQuint:
            return Tangram::EaseType::quint;
        case TGEaseTypeCubic:
            return Tangram::EaseType::cubic;
        default:
            return Tangram::EaseType::cubic;
    }
}

+ (TGError)convertTGErrorTypeFrom:(Tangram::Error)error
{
    switch (error) {
        case Tangram::Error::scene_update_path_yaml_syntax_error:
            return TGErrorSceneUpdatePathYAMLSyntaxError;
        case Tangram::Error::scene_update_path_not_found:
            return TGErrorSceneUpdatePathNotFound;
        case Tangram::Error::scene_update_value_yaml_syntax_error:
            return TGErrorSceneUpdateValueYAMLSyntaxError;
    }
}

+ (NSError *)errorFromSceneUpdateError:(Tangram::SceneUpdateError)updateError
{
    NSString* path = [NSString stringWithUTF8String:updateError.update.path.c_str()];
    NSString* value = [NSString stringWithUTF8String:updateError.update.value.c_str()];
    TGSceneUpdate* udpate = [[TGSceneUpdate alloc] initWithPath:path value:value];

    NSMutableDictionary* userInfo = [[NSMutableDictionary alloc] init];
    [userInfo setObject:udpate forKey:@"TGUpdate"];

    NSError* error = [NSError errorWithDomain:TGErrorDomain
                                         code:(NSInteger)updateError.error
                                     userInfo:userInfo];

    return error;
}

@end
