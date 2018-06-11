//
//  TGSceneUpdate.h
//  TangramMap
//
//  Created by Karim Naaji on 11/05/16.
//  Copyright (c) 2017 Mapzen. All rights reserved.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

/**
 Represents a data structure to specify a YAML path and the corresponding value for a Tangram scene update.

 Example to update an API key for a source defined like this in your stylesheet:

 ```
 sources:
     osm:
         type: MVT
         url:  https://tile.mapzen.com/mapzen/vector/v1/all/{z}/{x}/{y}.mvt
         max_zoom: 16
         url_params:
             api_key: vector-tiles-tyHL4AY
 ```

 ```swift
 view.queueSceneUpdates(sceneUpdates: [ TGSceneUpdate(path: "sources.osm.url_params", value: "{ api_key: \(YOUR_API_KEY) }") ])
 view.applySceneUpdates()
 ```
 */
@interface TGSceneUpdate : NSObject

/**
 Init a scene update for a specific YAML path in the stylesheet and a new value:

 @param path the YAML path within the stylesheet
 @param value the new YAML value for the node selected by path

 @return an initialized scene update
 */
- (instancetype)initWithPath:(NSString *)path value:(NSString *)value;

/// The YAML path for which the update will occur in the stylesheet used in the map view
@property (copy, nonatomic) NSString* path;

/// The YAML value to set in the update
@property (copy, nonatomic) NSString* value;

NS_ASSUME_NONNULL_END

@end
