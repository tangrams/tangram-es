//
//  TGGeoPolygon.h
//  tangram
//
//  Created by Karim Naaji on 10/27/16.
//
//

#import <Foundation/Foundation.h>
#import "TGGeoPoint.h"

@interface TGGeoPolygon : NSObject

- (id)init;

- (void)startPath:(TGGeoPoint)latlon;

- (void)startPath:(TGGeoPoint)latlon withSize:(unsigned int)size;

- (void)addPoint:(TGGeoPoint)latlon;

- (void*)data;

- (unsigned int)count;

- (void)removeAll;

@end
