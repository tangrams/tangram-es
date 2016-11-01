//
//  CGFontConverter.h
//  tangram
//
//  Created by Karim Naaji on 11/01/16.
//
//

#import <Foundation/Foundation.h>
#import <CoreGraphics/CoreGraphics.h>

@interface CGFontConverter : NSObject

+ (NSData *)fontDataForCGFont:(CGFontRef)cgFont;

@end


