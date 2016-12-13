//
//  TGGeoPolyline.h
//  tangram
//
//  Created by Karim Naaji on 10/27/16.
//
//

#import <Foundation/Foundation.h>
#import "TGGeoPoint.h"

@interface TGGeoPolyline : NSObject

- (instancetype)initWithSize:(unsigned int)size;

- (void)addPoint:(TGGeoPoint)latlon;

- (unsigned int)count;

- (TGGeoPoint*)coordinates;

- (void)removeAll;

@end
