#import "appleAllowedFonts.h"

bool allowedFamily(NSString* familyName) {
    const NSArray<NSString *> *allowedFamilyList = @[ @"Hebrew", @"Kohinoor", @"Gumurki", @"Thonburi", @"Tamil",
                                                    @"Gurmukhi", @"Kailasa", @"Sangam", @"PingFang", @"Geeza",
                                                    @"Mishafi", @"Farah", @"Hiragino", @"Gothic" ];

    for (NSString* allowedFamily in allowedFamilyList) {
        if ( [familyName containsString:allowedFamily] ) { return true; }
    }
    return false;
}
