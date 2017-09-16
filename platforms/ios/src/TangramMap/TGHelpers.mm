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
        case Tangram::Error::no_valid_scene:
            return TGErrorNoValidScene;
        case Tangram::Error::none:
            return TGErrorNone;
    }
}

+ (NSError *)errorFromSceneError:(Tangram::SceneError)updateError
{
    NSString* path = [NSString stringWithUTF8String:updateError.update.path.c_str()];
    NSString* value = [NSString stringWithUTF8String:updateError.update.value.c_str()];
    TGSceneUpdate* update = [[TGSceneUpdate alloc] initWithPath:path value:value];

    NSDictionary *userInfo = @{
        NSLocalizedDescriptionKey: NSLocalizedString((
            [NSString stringWithFormat:@"An error occured during Scene Update -> (%@: %@)", update.path, update.value]), nil),
        NSLocalizedFailureReasonErrorKey: NSLocalizedString(
            @"Possible bad yaml reference in the SceneUpdate object or within the scene", nil),
        NSLocalizedRecoverySuggestionErrorKey: NSLocalizedString(
            @"Try checking SceneUpdate path and value parameters, and making sure they conform to be valid yaml objects", nil),
    };

    NSError* error = [NSError errorWithDomain:TGErrorDomain
                                         code:(NSInteger)updateError.error
                                     userInfo:userInfo];

    return error;
}

@end
