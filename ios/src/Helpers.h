//
//  helpers.h
//  tangram
//
//  Created by Karim Naaji on 10/18/16.
//
//

#import "TGMapViewController.h"
#import "tangram.h"

@interface Helpers : NSObject 

+ (Tangram::EaseType)convertEaseTypeFrom:(TGEaseType)ease;

@end


