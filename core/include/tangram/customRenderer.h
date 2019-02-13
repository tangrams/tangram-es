#pragma once

namespace Tangram {

struct CustomRenderContext {
    double width = 0;
    double height = 0;
    double longitude = 0;
    double latitude = 0;
    double zoom = 0;
    double rotation = 0;
    double tilt = 0;
    double fieldOfView = 0;
};

class CustomRenderer {

public:

    virtual ~CustomRenderer() = default;

    // Called when the renderer is added to the map.
    virtual void initialize() = 0;

    // Called on the GL thread during each rendered frame.
    virtual void render(const CustomRenderContext& context) = 0;

    // Called when the map is destroyed or the renderer is removed.
    virtual void deinitialize() = 0;
};

} // namespace Tangram
