#include "data/tileData.h"
#include "marker/markerManager.h"
#include "marker/marker.h"
#include "scene/sceneLoader.h"
#include "style/style.h"
#include "log.h"

namespace Tangram {

void MarkerManager::setScene(std::shared_ptr<Scene> scene) {

    m_scene = scene;
    m_mapProjection = scene->mapProjection().get();
    m_styleContext.initFunctions(*scene);
    m_jsFnIndex = scene->functions().size();


    // Initialize StyleBuilders.
    for (auto& style : scene->styles()) {
        m_styleBuilders[style->getName()] = style->createBuilder();
    }

    // Rebuild any markers present.
    for (auto& entry : m_markers) {
        buildStyling(*entry);
        buildGeometry(*entry, m_zoom);
    }

}

MarkerID MarkerManager::add() {

    // Add a new empty marker object to the list of markers.
    auto id = ++m_idCounter;
    m_markers.push_back(std::make_unique<Marker>(id));

    // Return a handle for the marker.
    return id;

}

bool MarkerManager::remove(MarkerID markerID) {
    for (auto it = m_markers.begin(), end = m_markers.end(); it != end; ++it) {
        if (it->get()->id() == markerID) {
            m_markers.erase(it);
            return true;
        }
    }
    return false;
}

bool MarkerManager::setStyling(MarkerID markerID, const char* styling) {
    Marker* marker = getMarkerOrNull(markerID);
    if (!marker) { return false; }

    marker->setStylingString(std::string(styling));

    // Create a draw rule from the styling string.
    buildStyling(*marker);

    // Build the feature mesh for the marker's current geometry.
    buildGeometry(*marker, m_zoom);
    return true;
}

bool MarkerManager::setVisible(MarkerID markerID, bool visible) {
    Marker* marker = getMarkerOrNull(markerID);
    if (!marker) { return false; }

    marker->setVisible(visible);
    return true;

}

bool MarkerManager::setPoint(MarkerID markerID, LngLat lngLat) {
    Marker* marker = getMarkerOrNull(markerID);
    if (!marker) { return false; }

    // If the marker does not have a 'point' feature mesh built, build it.
    if (!marker->mesh() || !marker->feature() || marker->feature()->geometryType != GeometryType::points) {
        auto feature = std::make_unique<Feature>();
        feature->geometryType = GeometryType::points;
        feature->points.emplace_back();
        marker->setFeature(std::move(feature));
        buildGeometry(*marker, m_zoom);
    }

    // Update the marker's bounds to the given coordinates.
    auto origin = m_mapProjection->LonLatToMeters({ lngLat.longitude, lngLat.latitude });
    marker->setBounds({ origin, origin });

    return true;
}

bool MarkerManager::setPointEased(MarkerID markerID, LngLat lngLat, float duration, EaseType ease) {
    Marker* marker = getMarkerOrNull(markerID);
    if (!marker) { return false; }

    // If the marker does not have a 'point' feature built, set that point immediately.
    if (!marker->mesh() || !marker->feature() || marker->feature()->geometryType != GeometryType::points) {
        return setPoint(markerID, lngLat);
    }

    auto dest = m_mapProjection->LonLatToMeters({ lngLat.longitude, lngLat.latitude });
    marker->setEase(dest, duration, ease);

    return true;
}

bool MarkerManager::setPolyline(MarkerID markerID, LngLat* coordinates, int count) {
    Marker* marker = getMarkerOrNull(markerID);
    if (!marker) { return false; }
    if (!coordinates || count < 2) { return false; }

    // Build a feature for the new set of polyline points.
    auto feature = std::make_unique<Feature>();
    feature->geometryType = GeometryType::lines;
    feature->lines.emplace_back();
    auto& line = feature->lines.back();

    // Determine the bounds of the polyline.
    BoundingBox bounds;
    bounds.min = { coordinates[0].longitude, coordinates[0].latitude };
    bounds.max = bounds.min;
    for (int i = 0; i < count; ++i) {
        bounds.expand(coordinates[i].longitude, coordinates[i].latitude);
    }
    bounds.min = m_mapProjection->LonLatToMeters(bounds.min);
    bounds.max = m_mapProjection->LonLatToMeters(bounds.max);

    // Update the marker's bounds.
    marker->setBounds(bounds);

    float scale = 1.f / marker->extent();

    // Project and offset the coordinates into the marker-local coordinate system.
    auto origin = marker->origin(); // SW corner.
    for (int i = 0; i < count; ++i) {
        auto degrees = glm::dvec2(coordinates[i].longitude, coordinates[i].latitude);
        auto meters = m_mapProjection->LonLatToMeters(degrees);
        line.emplace_back((meters.x - origin.x) * scale, (meters.y - origin.y) * scale, 0.f);
    }

    // Update the feature data for the marker.
    marker->setFeature(std::move(feature));

    // Build a new mesh for the marker.
    buildGeometry(*marker, m_zoom);

    return true;
}

bool MarkerManager::setPolygon(MarkerID markerID, LngLat* coordinates, int* counts, int rings) {
    Marker* marker = getMarkerOrNull(markerID);
    if (!marker) { return false; }
    if (!coordinates || !counts || rings < 1) { return false; }

    // Build a feature for the new set of polygon points.
    auto feature = std::make_unique<Feature>();
    feature->geometryType = GeometryType::polygons;
    feature->polygons.emplace_back();
    auto& polygon = feature->polygons.back();

    // Determine the bounds of the polygon.
    BoundingBox bounds;
    LngLat* ring = coordinates;
    for (int i = 0; i < rings; ++i) {
        int count = counts[i];
        for (int j = 0; j < count; ++j) {
            if (i == 0 && j == 0) {
                bounds.min = { ring[0].longitude, ring[0].latitude };
                bounds.max = bounds.min;
            }
            bounds.expand(ring[j].longitude, ring[j].latitude);
        }
        ring += count;
    }
    bounds.min = m_mapProjection->LonLatToMeters(bounds.min);
    bounds.max = m_mapProjection->LonLatToMeters(bounds.max);

    // Update the marker's bounds.
    marker->setBounds(bounds);

    float scale = 1.f / marker->extent();

    // Project and offset the coordinates into the marker-local coordinate system.
    auto origin = marker->origin(); // SW corner.
    ring = coordinates;
    for (int i = 0; i < rings; ++i) {
        int count = counts[i];
        polygon.emplace_back();
        auto& line = polygon.back();
        for (int j = 0; j < count; ++j) {
            auto degrees = glm::dvec2(ring[j].longitude, ring[j].latitude);
            auto meters = m_mapProjection->LonLatToMeters(degrees);
            line.emplace_back((meters.x - origin.x) * scale, (meters.y - origin.y) * scale, 0.f);
        }
        ring += count;
    }

    // Update the feature data for the marker.
    marker->setFeature(std::move(feature));

    // Build a new mesh for the marker.
    buildGeometry(*marker, m_zoom);

    return true;
}

bool MarkerManager::update(int zoom) {

    if (zoom == m_zoom) {
         return false;
    }
    bool rebuilt = false;
    for (auto& marker : m_markers) {
        if (zoom != marker->builtZoomLevel()) {
            buildGeometry(*marker, zoom);
            rebuilt = true;
        }
    }
    m_zoom = zoom;
    return rebuilt;
}

void MarkerManager::removeAll() {

    m_markers.clear();

}

const std::vector<std::unique_ptr<Marker>>& MarkerManager::markers() const {
    return m_markers;
}

void MarkerManager::buildStyling(Marker& marker) {

    // Update the draw rule for the marker.
    YAML::Node node = YAML::Load(marker.stylingString());
    std::vector<StyleParam> params;
    SceneLoader::parseStyleParams(node, m_scene, "", params);

    // Compile any new JS functions used for styling.
    const auto& sceneJsFnList = m_scene->functions();
    for (auto i = m_jsFnIndex; i < sceneJsFnList.size(); ++i) {
        m_styleContext.addFunction(sceneJsFnList[i]);
    }
    m_jsFnIndex = sceneJsFnList.size();

    marker.setDrawRule(std::make_unique<DrawRuleData>("", 0, std::move(params)));

}

void MarkerManager::buildGeometry(Marker& marker, int zoom) {

    auto feature = marker.feature();
    auto rule = marker.drawRule();
    if (!feature || !rule) { return; }

    StyleBuilder* styler = nullptr;
    {
        auto name = rule->getStyleName();
        auto it = m_styleBuilders.find(name);
        if (it != m_styleBuilders.end()) {
            styler = it->second.get();
        } else {
            LOGN("Invalid style %s", name.c_str());
            return;
        }
    }

    m_styleContext.setKeywordZoom(zoom);

    bool valid = m_ruleSet.evaluateRuleForContext(*rule, m_styleContext);

    if (valid) {
        styler->setup(marker, zoom);
        styler->addFeature(*feature, *rule);
        marker.setMesh(styler->style().getID(), zoom, styler->build());
    }

}

Marker* MarkerManager::getMarkerOrNull(MarkerID markerID) {
    if (!markerID) { return nullptr; }
    for (const auto& entry : m_markers) {
        if (entry->id() == markerID) { return entry.get(); }
    }
    return nullptr;
}

} // namespace Tangram
