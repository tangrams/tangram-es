#pragma once

namespace Tangram {

struct MapPoint {
    double x = 0;
    double y = 0;
};

class AnnotationViewState {

public:

    MapPoint convertLngLatToProjectedMeters(double longitude, double latitude) const;

    MapPoint getViewOriginInProjectedMeters() const;

    float* getViewProjectionMatrix() const;

};

class AnnotationRenderer {

public:

    virtual ~AnnotationRenderer() = default;

    // Called when the renderer is added to the map.
    virtual void initialize() = 0;

    // Called on the GL thread during each rendered frame.
    virtual void render(const AnnotationViewState& context) = 0;

    // Called when the map is destroyed or the renderer is removed.
    virtual void deinitialize() = 0;

};

} // namespace Tangram
