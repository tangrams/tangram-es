//
//  TGTypes.mm
//  TangramMap
//
//  Created by Karim Naaji on 4/20/17.
//  Copyright (c) 2017 Mapzen. All rights reserved.
//

#import "TGTypes.h"
#import "TGTypes+Internal.h"

NSString* const TGErrorDomain = @"TGErrorDomain";

NSError* TGConvertCoreSceneErrorToNSError(const Tangram::SceneError *sceneError) {
    if (sceneError == nullptr) {
        return nil;
    }

    NSString* path = [NSString stringWithUTF8String:sceneError->update.path.c_str()];
    NSString* value = [NSString stringWithUTF8String:sceneError->update.value.c_str()];

    NSDictionary *userInfo = @{
                               NSLocalizedDescriptionKey: NSLocalizedString(([NSString stringWithFormat:@"An error occured during Scene Update -> (%@: %@)", path, value]), nil),
                               NSLocalizedFailureReasonErrorKey: NSLocalizedString(@"Possible bad yaml reference in the SceneUpdate object or within the scene", nil),
                               NSLocalizedRecoverySuggestionErrorKey: NSLocalizedString(@"Try checking SceneUpdate path and value parameters, and making sure they conform to be valid yaml objects", nil),
                               };

    NSError* error = [NSError errorWithDomain:TGErrorDomain
                                         code:(NSInteger)sceneError->error
                                     userInfo:userInfo];

    return error;
}
