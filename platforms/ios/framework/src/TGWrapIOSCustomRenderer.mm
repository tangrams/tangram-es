//
//  TGWrapIOSCustomRenderer.mm
//  tangram
//
//  Created by Varun Talwar on 7/19/18.
//

#include <memory>

#import "IOSCustomRenderer.h"
#import "TGWrapIOSCustomRenderer.h"
#import <Foundation/Foundation.h>

@implementation TGWrapIOSCustomRenderer {
    std::unique_ptr<Tangram::IOSCustomRenderer> iosCustomRenderer;
}

- (instancetype)initWithTGCustomRenderer:(id<TGCustomRenderer>)tgCustomRenderer
{
    iosCustomRenderer = std::make_unique<Tangram::IOSCustomRenderer>(tgCustomRenderer);
    return self;
}

- (Tangram::CustomRenderer*)getNativeCustomRenderer {
    return iosCustomRenderer.get();
}

@end
