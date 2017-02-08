//
//  TGFontConverter.h
//  tangram
//
//  Created by Karim Naaji on 11/01/16.
//
//

#import <Foundation/Foundation.h>
#import <CoreGraphics/CoreGraphics.h>

#import <vector>

@interface TGFontConverter : NSObject

+ (std::vector<char>)fontDataForCGFont:(CGFontRef)cgFont;

@end


