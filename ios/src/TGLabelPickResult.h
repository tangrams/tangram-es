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

NS_ASSUME_NONNULL_BEGIN

- (id) initWithCoordinates:(TGGeoPoint)coordinates type:(TGLabelType)type properties:(NSDictionary *)properties;

@property (readonly, nonatomic) TGGeoPoint coordinates;
@property (readonly, nonatomic) TGLabelType type;
@property (readonly, nonatomic) NSDictionary* properties;

NS_ASSUME_NONNULL_END

@end
