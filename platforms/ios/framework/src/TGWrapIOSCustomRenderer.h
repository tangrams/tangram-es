//
//  TGWrapIOSCustomRenderer.h
//  tangram
//
//  Created by Varun Talwar on 7/13/18.
//

#import <Foundation/Foundation.h>

namespace Tangram {
class CustomRenderer;
}
@protocol TGCustomRenderer;

/**
 A wrapper helper class around `IOSCustomRenderer` to interface between objC and C++
 */
@interface TGWrapIOSCustomRenderer : NSObject

/**
 Initialize a wrapped IOS Custom Renderer instance with an instance of `TGCustomRenderer`

 @return an initialized Wrapped IOS Custom Renderer
 */
- (instancetype)initWithTGCustomRenderer:(id<TGCustomRenderer>)tgCustomRenderer;

/**
 Exposes raw pointer to `Tangram::CustomRenderer` to be used for core tangram map instance
 See:

 `-[TGMapView addCustomRenderer:anchorBeforeLayer:]`

 `-[TGMapView removeCustomRenderer:]`

 @return raw pointer to `Tangram::CustomRenderer`
 */
- (Tangram::CustomRenderer*)getNativeCustomRenderer;

@end // interface TGWrapIOSCustomRenderer
