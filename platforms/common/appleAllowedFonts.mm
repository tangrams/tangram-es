#import "appleAllowedFonts.h"

namespace Tangram {

bool allowedFamily(NSString* familyName) {
    // macOS and iOS have many pre-installed fonts, supporting many scripts.
    // Evaluating all fonts to find glyphs is slow, so limit the set of fonts loaded to one family per script.
    // Apple provides a list of pre-installed fonts on the latest versions of macOS and iOS. These lists may not
    // reflect the fonts available on earlier OS versions, so check multiple versions when possible.
    // https://developer.apple.com/fonts/system-fonts/#preinstalled
    const NSArray<NSString *> *allowedFamilyList =
            @[ @"Apple SD Gothic Neo", // Hangul
               @"Arial Hebrew", // Hebrew
               @"Geeza Pro", // Arabic
               @"Gurmukhi", // Gurmukhi
               @"Hiragino", // Hiragana, Katakana
               @"Kailasa", // Tibetan
               @"Kannada Sangam", // Kannada
               @"Kohinoor", // Bangla, Devanagari, Telugu
               @"Malayalam Sangam", // Malayalam
               @"Mishafi", // Arabic, Najdi Arabic
               @"Myanmar Sangam", // Myanmar
               @"Noto Nastaliq Urdu", // Arabic, Urdu
               @"Oriya Sangam", // Odia
               @"PingFang", // Hiragana, Katakana, Simplified Han, Traditional Han
               @"Tamil", // Tamil
               @"Thonburi", // Cyrillic, Thai
            ];

    for (NSString* allowedFamily in allowedFamilyList) {
        if ( [familyName containsString:allowedFamily] ) { return true; }
    }
    return false;
}

}
