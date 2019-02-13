//
//  TGCustomRenderer.h
//  tangram
//
//  Created by Varun Talwar on 7/13/18.
//

#import <Foundation/Foundation.h>

/**
 Structure holding rendering context for `TGCustomRenderer`
 Mimics structure of `Tangram::CustomRendererContext`
 */
typedef struct TGCustomRendererContext {
    double width;
    double height;
    double longitude;
    double latitude;
    double zoom;
    double rotation;
    double tilt;
    double fieldOfView;
} TGCustomRendererContext;

/**
 CustomRenderer protocol catering client specific rendering logic

 @note protocol methods are called internally by `Tangram::CustomStyle`
 */
@protocol TGCustomRenderer <NSObject>

/**
 Configures `TGCustomRenderer` instance during initialization, when added to the map

 See:

 `-[TGMapView addCustomRenderer:anchorBeforeLayer:]`
 */
- (void)prepare;

/**
 Performs rendering tasks associated with the `TGCustomRenderer` instance on the GL Thread

 @param context specifying `TGCustomRendererContext` for the rendering
 */
- (void)drawWithContext:(TGCustomRendererContext)context;

/**
 Wraps up `TGCustomRenderer` instance when removed from the map.

 See:

 `-[TGMapView removeCustomRenderer:]`
 */
- (void)complete;

@end // protocol TGCustomRenderer
