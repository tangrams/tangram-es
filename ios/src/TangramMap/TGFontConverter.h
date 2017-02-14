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

NS_ASSUME_NONNULL_BEGIN

@interface TGFontConverter : NSObject

+ (std::vector<char>)fontDataForCGFont:(CGFontRef)cgFont;

NS_ASSUME_NONNULL_END

@end


