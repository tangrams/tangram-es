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

- (void)startPath:(TGGeoPoint)latlon;

- (void)startPath:(TGGeoPoint)latlon withSize:(unsigned int)size;

- (void)addPoint:(TGGeoPoint)latlon;

- (TGGeoPoint*)coordinates;

- (int*)rings;

- (unsigned int)count;

- (unsigned int)ringsCount;

- (void)removeAll;

@end
