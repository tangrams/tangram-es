//
//  IOSCustomRenderer.h
//  tangram
//
//  Created by Varun Talwar on 7/19/18.
//

#include "customRenderer.h"

#import <Foundation/Foundation.h>

@protocol TGCustomRenderer;

namespace Tangram {

/**
 IOSCustomRenderer class which implements abstract methods for `Tangram::CustomRenderer` for iOS purposes.
 */
class IOSCustomRenderer : public CustomRenderer {
    id<TGCustomRenderer> m_tgCustomRenderer;

public:
    IOSCustomRenderer(id<TGCustomRenderer> customRenderer);

    // Called when the renderer is added to the map.
    void initialize() override;

    // Called on the GL thread during each rendered frame.
    void render(const CustomRenderContext& context) override;

    // Called when the map is destroyed or the renderer is removed.
    void deinitialize() override;
}; // IOSCustomRenderer class

}
