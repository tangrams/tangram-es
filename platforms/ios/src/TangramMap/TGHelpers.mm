//
//  TGHelpers.m
//  TangramMap
//
//  Created by Karim Naaji on 10/18/16.
//  Copyright (c) 2017 Mapzen. All rights reserved.
//

#import "TGHelpers.h"

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

+ (TGSceneUpdateError)convertSceneUpdateErrorTypeFrom:(Tangram::SceneUpdateError)error
{
    switch (error) {
        case Tangram::SceneUpdateError::path_yaml_syntax_error:
            return TGSceneUpdateErrorPathYAMLSyntaxError;
        case Tangram::SceneUpdateError::path_not_found:
            return TGSceneUpdateErrorPathNotFound;
        case Tangram::SceneUpdateError::value_yaml_syntax_error:
            return TGSceneUpdateErrorValueYamlSyntaxError;
    }
}

@end
