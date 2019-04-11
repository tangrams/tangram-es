//
//  TGLabelPickResult.h
//  TangramMap
//
//  Created by Karim Naaji on 11/02/16.
//  Updated by Matt Blair on 8/21/18.
//  Copyright (c) 2017 Mapzen. All rights reserved.
//

#import <CoreLocation/CoreLocation.h>
#import <Foundation/Foundation.h>
#import "TGExport.h"
#import "TGMapData.h"

/**
 A label type used to differentiate icon and text labels when selecting them on a map view
 */
typedef NS_ENUM(NSInteger, TGLabelType) {
    /// A type for icon labels (sprites)
    TGLabelTypeIcon = 0,
    /// A type for text labels
    TGLabelTypeText,
};

/**
 Data structure holding the result of a label selection that occured on the map view.

 See `-[TGMapView pickLabelAt:]` and `[TGMapViewDelegate mapView:didSelectLabel:atScreenPosition:]`.
 */
TG_EXPORT
@interface TGLabelPickResult : NSObject

NS_ASSUME_NONNULL_BEGIN

/// The geographic coordinates of the selected label
@property (readonly, nonatomic) CLLocationCoordinate2D coordinate;

/// The type of the label (text or icon)
@property (readonly, nonatomic) TGLabelType type;

/// The set of data properties attached with the label
@property (readonly, strong, nonatomic) TGFeatureProperties* properties;

NS_ASSUME_NONNULL_END

@end
