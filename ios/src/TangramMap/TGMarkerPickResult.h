//
//  TGMarkerPickResult.h
//  tangram
//
//  Created by Karim Naaji on 01/03/17.
//

#import <Foundation/Foundation.h>
#import "TGGeoPoint.h"

/**
 A marker identifier for use in the <a href="Classes/TGMapViewController.html#/Marker%20interface">marker interface</a>.
 A marker identifier of 0 is non-valid.
 */
typedef uint32_t TGMapMarkerId;

@interface TGMarkerPickResult : NSObject

- (instancetype) initWithCoordinates:(TGGeoPoint)coordinates identifier:(TGMapMarkerId)identifier;

@property (readonly, nonatomic) TGGeoPoint coordinates;
@property (readonly, nonatomic) TGMapMarkerId identifier;

@end
