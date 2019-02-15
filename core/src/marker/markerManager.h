#pragma once

#include "scene/drawRule.h"
#include "scene/scene.h"
#include "util/ease.h"
#include "util/fastmap.h"
#include "util/types.h"

#include <memory>
#include <vector>

namespace Tangram {

class MapProjection;
class Marker;
class StyleBuilder;
class StyleContext;
class View;

class MarkerManager {

public:

    explicit MarkerManager(const Scene& scene);
    ~MarkerManager();

    // Create a new, empty marker and return its ID. An ID of 0 indicates an invalid marker.
    MarkerID add();

    // Try to remove the marker with the given ID; returns true if the marker was found and removed.
    bool remove(MarkerID markerID);

    // Set the styling for a marker using a YAML string; returns true if the marker was found and updated.
    bool setStylingFromString(MarkerID markerID, const char* styling) {
        return setStyling(markerID, styling, false);
    }

    // Set the styling for a marker using a scene path; returns true is the marker was found and update.
    bool setStylingFromPath(MarkerID markerID, const char* path) {
        return setStyling(markerID, path, true);
    }

    bool setBitmap(MarkerID markerID, int width, int height, float density, const unsigned int* bitmapData);

    // Set whether a marker should be visible; returns true if the marker was found and updated.
    bool setVisible(MarkerID markerID, bool visible);

    // Set the ordering of this marker relative to other markers. Higher values are drawn 'above' others.
    bool setDrawOrder(MarkerID markerID, int drawOrder);

    // Set a marker to a point feature at the given position; returns true if the marker was found and updated.
    bool setPoint(MarkerID markerID, LngLat lngLat);

    // Set a marker to a point feature at the given position; if the marker was previously set to a point, this
    // eases from the old position to the new one over the given duration with the given ease type; returns true if
    // the marker was found and updated.
    bool setPointEased(MarkerID markerID, LngLat lngLat, float duration, EaseType ease);

    // Set a marker to a polyline feature at the given position; returns true if the marker was found and updated.
    bool setPolyline(MarkerID markerID, LngLat* coordinates, int count);

    // Set a marker to a polygon feature at the given position; returns true if the marker was found and updated.
    bool setPolygon(MarkerID markerID, LngLat* coordinates, int* counts, int rings);

    // Update the zoom level for all markers; markers are built for one zoom
    // level at a time so when the current zoom changes, all marker meshes are
    // rebuilt. Returns true when any Markers changed since last call to update.
    bool update(const View& _view, float _dt);

    // Remove and destroy all markers.
    void removeAll();

    // Rebuild all markers.
    void rebuildAll();

    const std::vector<std::unique_ptr<Marker>>& markers() const;

    const Marker* getMarkerOrNullBySelectionColor(uint32_t selectionColor) const;

private:

    Marker* getMarkerOrNull(MarkerID markerID);

    bool setStyling(MarkerID markerID, const char* styling, bool isPath);
    bool buildStyling(Marker& marker);
    bool buildMesh(Marker& marker, int zoom);

    const Scene& m_scene;
    // Custom functions and stops from styling strings
    SceneStops m_stops;
    SceneFunctions m_functions;

    std::unique_ptr<StyleContext> m_styleContext;
    std::vector<std::unique_ptr<Marker>> m_markers;
    std::vector<std::string> m_jsFnList;
    fastmap<std::string, std::unique_ptr<StyleBuilder>> m_styleBuilders;

    uint32_t m_idCounter = 0;
    int m_zoom = 0;
    bool m_dirty = false;

};

} // namespace Tangram
