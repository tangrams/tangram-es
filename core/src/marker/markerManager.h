#pragma once

#include "scene/styleContext.h"
#include "scene/drawRule.h"
#include "util/ease.h"
#include "util/fastmap.h"
#include "util/types.h"
#include <memory>
#include <vector>

namespace Tangram {

class MapProjection;
class Marker;
class Scene;
class StyleBuilder;

class MarkerManager {

public:

    void setScene(std::shared_ptr<Scene> scene);

    MarkerID add();

    bool remove(MarkerID markerID);

    bool setStyling(MarkerID markerID, const char* styling);

    bool setVisible(MarkerID markerID, bool visible);

    bool setPoint(MarkerID markerID, LngLat lngLat);

    bool setPointEased(MarkerID markerID, LngLat lngLat, float duration, EaseType ease);

    bool setPolyline(MarkerID markerID, LngLat* coordinates, int count);

    bool setPolygon(MarkerID markerID, LngLat* coordinates, int* counts, int rings);

    bool update(int zoom);

    void removeAll();

    const std::vector<std::unique_ptr<Marker>>& markers() const;

private:

    Marker* tryGet(MarkerID markerID);

    void build(Marker& marker, int zoom);

    DrawRuleMergeSet m_ruleSet;
    StyleContext m_styleContext;
    std::shared_ptr<Scene> m_scene;
    std::vector<std::unique_ptr<Marker>> m_markers;
    std::vector<std::string> m_jsFnList;
    fastmap<std::string, std::unique_ptr<StyleBuilder>> m_styleBuilders;
    MapProjection* m_mapProjection = nullptr;
    size_t m_jsFnIndex = 0;
    uint32_t m_idCounter = 0;
    int m_zoom = 0;

};

} // namespace Tangram
