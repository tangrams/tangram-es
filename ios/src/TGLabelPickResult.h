//
//  TGLabelPickResult.h
//  tangram
//
//  Created by Karim Naaji on 11/02/16.
//

#import <Foundation/Foundation.h>
#import "TGGeoPoint.h"

typedef NS_ENUM(NSInteger, TGLabelType) {
    TGLabelTypeIcon = 0,
    TGLabelTypeText,
};

@interface TGLabelPickResult : NSObject

- (instancetype) initWithCoordinates:(TGGeoPoint)coordinates type:(TGLabelType)type properties:(nonnull NSDictionary *)properties;

@property (readonly, nonatomic) TGGeoPoint coordinates;
@property (readonly, nonatomic) TGLabelType type;
@property (readonly, strong, nonatomic) NSDictionary* properties;

@end
