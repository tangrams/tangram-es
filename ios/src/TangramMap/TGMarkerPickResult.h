//
//  TGMarkerPickResult.h
//  tangram
//
//  Created by Karim Naaji on 01/03/17.
//

#import <Foundation/Foundation.h>
#import "TGGeoPoint.h"

typedef uint32_t TGMapMarkerId;

@interface TGMarkerPickResult : NSObject

- (instancetype) initWithCoordinates:(TGGeoPoint)coordinates identifier:(TGMapMarkerId)identifier;

@property (readonly, nonatomic) TGGeoPoint coordinates;
@property (readonly, nonatomic) TGMapMarkerId identifier;

@end
