//
//  helpers.m
//  tangram
//
//  Created by Karim Naaji on 10/18/16.
//
//

#import "Helpers.h"

@implementation Helpers

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

@end
