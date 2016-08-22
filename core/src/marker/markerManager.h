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

    Marker* add(const char* styling);

    bool remove(Marker* marker);

    bool setStyling(Marker* marker, const char* styling);

    bool setPoint(Marker* marker, LngLat lngLat);

    bool setPointEased(Marker* marker, LngLat lngLat, float duration, EaseType ease);

    bool setPolyline(Marker* marker, LngLat* coordinates, int count);

    bool setPolygon(Marker* marker, LngLat* coordinates, int* counts, int rings);

    const std::vector<std::unique_ptr<Marker>>& markers() const;

private:

    bool contains(Marker* marker);

    void build(Marker& _marker);

    DrawRuleMergeSet m_ruleSet;
    StyleContext m_styleContext;
    std::shared_ptr<Scene> m_scene;
    std::vector<std::unique_ptr<Marker>> m_markers;
    fastmap<std::string, std::unique_ptr<StyleBuilder>> m_styleBuilders;
    MapProjection* m_mapProjection = nullptr;

};

} // namespace Tangram
