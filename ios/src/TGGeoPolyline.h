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

- (id)initWithSize:(unsigned int)size;

- (void)addPoint:(TGGeoPoint)latlon;

- (unsigned int)count;

- (TGGeoPoint*)coordinates;

- (void)removeAll;

@end
