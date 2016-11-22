//
//  TGFontConverter.h
//  tangram
//
//  Created by Karim Naaji on 11/01/16.
//
//

#import <Foundation/Foundation.h>
#import <CoreGraphics/CoreGraphics.h>

@interface TGFontConverter : NSObject

+ (unsigned char *)fontDataForCGFont:(CGFontRef)cgFont size:(size_t *)size;

@end


