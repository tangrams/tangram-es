#include "data/tileData.h"
#include "marker/markerManager.h"
#include "marker/marker.h"
#include "scene/sceneLoader.h"
#include "style/style.h"

namespace Tangram {

void MarkerManager::setScene(std::shared_ptr<Scene> scene) {

    m_scene = scene;
    m_mapProjection = scene->mapProjection().get();
    m_styleContext.initFunctions(*scene);

    // Initialize StyleBuilders
    for (auto& style : scene->styles()) {
        m_styleBuilders[style->getName()] = style->createBuilder();
    }

}

Marker* MarkerManager::add(const char* styling) {

    // Add a new empty marker object to the list of markers.
    m_markers.push_back(std::make_unique<Marker>());

    // Create a draw rule from the given styling string.
    auto marker = m_markers.back().get();
    setStyling(marker, styling);

    // Return a pointer to the marker.
    return marker;

}

bool MarkerManager::remove(Marker* marker) {
    for (auto it = m_markers.begin(), end = m_markers.end(); it != end; ++it) {
        if (it->get() == marker) {
            m_markers.erase(it);
            return true;
        }
    }
    return false;
}

bool MarkerManager::setStyling(Marker* marker, const char* styling) {
    if (!marker || !contains(marker)) { return false; }

    // Update the draw rule for the marker.
    YAML::Node node = YAML::Load(styling);
    std::vector<StyleParam> params;
    SceneLoader::parseStyleParams(node, m_scene, "", params);
    marker->setStyling(std::make_unique<DrawRuleData>("anonymous_marker_rule", 0, std::move(params)));

    // Build the feature mesh for the marker's current geometry.
    build(*marker);
    return true;
}

bool MarkerManager::setPoint(Marker* marker, double lng, double lat) {
    if (!marker || !contains(marker)) { return false; }

    // If the marker does not have a 'point' feature mesh built, build it.
    if (!marker->mesh() || !marker->feature() || marker->feature()->geometryType != GeometryType::points) {
        auto feature = std::make_unique<Feature>();
        feature->geometryType = GeometryType::points;
        feature->points.emplace_back();
        marker->setFeature(std::move(feature));
        build(*marker);
    }

    // Update the marker's bounds to the given coordinates.
    auto origin = m_mapProjection->LonLatToMeters({ lng, lat });
    marker->setBounds({ origin, origin });

    return true;
}

bool MarkerManager::setPolyline(Marker* marker, double* coordinates, int count) {
    if (!marker || !contains(marker)) { return false; }
    if (!coordinates || count < 2) { return false; }

    // Build a feature for the new set of polyline points.
    auto feature = std::make_unique<Feature>();
    feature->geometryType = GeometryType::lines;
    feature->lines.emplace_back();
    auto& line = feature->lines.back();

    // Determine the bounds of the polyline.
    BoundingBox bounds;
    bounds.min = { coordinates[0], coordinates[1] };
    bounds.max = bounds.min;
    for (int i = 0; i < count; ++i) {
        bounds.expand(coordinates[2 * i], coordinates[2 * i + 1]);
    }
    bounds.min = m_mapProjection->LonLatToMeters(bounds.min);
    bounds.max = m_mapProjection->LonLatToMeters(bounds.max);

    // Project and offset the coordinates into the marker-local coordinate system.
    auto origin = glm::dvec2(bounds.min.x, bounds.max.y); // SW corner.
    for (int i = 0; i < count; ++i) {
        auto degrees = glm::dvec2(coordinates[2 * i], coordinates[2 * i + 1]);
        auto meters = m_mapProjection->LonLatToMeters(degrees);
        line.emplace_back(meters.x - origin.x, meters.y - origin.y, 0.f);
    }

    // Update the feature data for the marker.
    marker->setFeature(std::move(feature));

    // Update the marker's bounds
    marker->setBounds(bounds);

    // Build a new mesh for the marker.
    build(*marker);

    return true;
}

bool MarkerManager::setPolygon(Marker* marker, double** coordinates, int* counts, int rings) {
    if (!marker || !contains(marker)) { return false; }
    if (!coordinates || !counts || rings < 1) { return false; }

    // Build a feature for the new set of polygon points.
    auto feature = std::make_unique<Feature>();
    feature->geometryType = GeometryType::polygons;
    feature->polygons.emplace_back();
    auto& polygon = feature->polygons.back();

    // Determine the bounds of the polygon.
    BoundingBox bounds;
    for (int i = 0; i < rings; ++i) {
        int count = counts[i];
        double* ring = coordinates[i];
        for (int j = 0; j < count; ++j) {
            if (i == 0 && j == 0) {
                bounds.min = { ring[0], ring[1] };
                bounds.max = bounds.min;
            }
            bounds.expand(ring[2 * j], ring[2 * j + 1]);
        }
    }
    bounds.min = m_mapProjection->LonLatToMeters(bounds.min);
    bounds.max = m_mapProjection->LonLatToMeters(bounds.max);

    // Project and offset the coordinates into the marker-local coordinate system.
    auto origin = glm::dvec2(bounds.min.x, bounds.max.y); // SW corner.
    for (int i = 0; i < rings; ++i) {
        int count = counts[i];
        double* ring = coordinates[i];
        polygon.emplace_back();
        auto& line = polygon.back();
        for (int j = 0; j < count; ++j) {
            auto degrees = glm::dvec2(ring[2 * j], ring[2 * j + 1]);
            auto meters = m_mapProjection->LonLatToMeters(degrees);
            line.emplace_back(meters.x - origin.x, meters.y - origin.y, 0.f);
        }
    }

    // Update the feature data for the marker.
    marker->setFeature(std::move(feature));

    // Update the marker's bounds
    marker->setBounds(bounds);

    // Build a new mesh for the marker.
    build(*marker);

    return true;
}

const std::vector<std::unique_ptr<Marker>>& MarkerManager::markers() const {
    return m_markers;
}

void MarkerManager::build(Marker& marker) {

    auto rule = marker.drawRule();
    auto feature = marker.feature();

    if (!rule || !feature) { return; }

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

    // TODO: setup geometry dimensions for style builder as in setup(Tile&)

    m_styleContext.setKeywordZoom(0); // TODO: use a meaningful zoom value

    bool valid = m_ruleSet.evaluateRuleForContext(*rule, m_styleContext);

    if (valid) {
        styler->setup(marker);
        styler->addFeature(*feature, *rule);
        marker.setMesh(styler->style().getID(), styler->build());
    }

}

bool MarkerManager::contains(Marker* marker) {
    for (auto it = m_markers.begin(), end = m_markers.end(); it != end; ++it) {
        if (it->get() == marker) { return true; }
    }
    return false;
}

} // namespace Tangram
