#include "marker/markerManager.h"

#include "data/tileData.h"
#include "gl/texture.h"
#include "marker/marker.h"
#include "scene/sceneLoader.h"
#include "scene/dataLayer.h"
#include "style/style.h"
#include "labels/labelSet.h"
#include "log.h"
#include "selection/featureSelection.h"

#include <algorithm>

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

    rebuildAll();
}

MarkerID MarkerManager::add() {

    // Add a new empty marker object to the list of markers.
    auto id = ++m_idCounter;
    m_markers.push_back(std::make_unique<Marker>(id));

    // Sort the marker list by draw order.
    std::stable_sort(m_markers.begin(), m_markers.end(), Marker::compareByDrawOrder);

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

bool MarkerManager::setStylingFromString(MarkerID markerID, const char* styling) {
    Marker* marker = getMarkerOrNull(markerID);
    if (!marker) { return false; }

    marker->setStyling(std::string(styling), false);

    // Create a draw rule from the styling string.
    if (!buildStyling(*marker)) { return false; }

    // Build the feature mesh for the marker's current geometry.
    return buildGeometry(*marker, m_zoom);
}

bool MarkerManager::setStylingFromPath(MarkerID markerID, const char* path) {
    Marker* marker = getMarkerOrNull(markerID);
    if (!marker) { return false; }

    marker->setStyling(std::string(path), true);

    // Create a draw rule from the styling string.
    if (!buildStyling(*marker)) { return false; }

    // Build the feature mesh for the marker's current geometry.
    return buildGeometry(*marker, m_zoom);
}

bool MarkerManager::setBitmap(MarkerID markerID, int width, int height, const unsigned int* bitmapData) {
    Marker* marker = getMarkerOrNull(markerID);
    if (!marker) { return false; }

    TextureOptions options = { GL_RGBA, GL_RGBA, { GL_LINEAR, GL_LINEAR }, { GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE } };
    auto texture = std::make_unique<Texture>(width, height, options);
    unsigned int size = width * height;
    texture->setData(bitmapData, size);

    marker->setTexture(std::move(texture));

    // The geometry is unchanged, but the mesh must be rebuilt because DynamicQuadMesh contains
    // texture batches as part of its data.
    buildGeometry(*marker, m_zoom);

    return true;
}

bool MarkerManager::setVisible(MarkerID markerID, bool visible) {
    Marker* marker = getMarkerOrNull(markerID);
    if (!marker) { return false; }

    auto labelMesh = dynamic_cast<const LabelSet*>(marker->mesh());
    if (labelMesh) {
        for (auto& label : labelMesh->getLabels()) {
            label->setAlpha(visible ? 1.0 : 0.0);
        }
    }

    marker->setVisible(visible);
    return true;
}

bool MarkerManager::setDrawOrder(MarkerID markerID, int drawOrder) {
    Marker* marker = getMarkerOrNull(markerID);
    if (!marker) { return false; }

    marker->setDrawOrder(drawOrder);

    // Sort the marker list by draw order.
    std::stable_sort(m_markers.begin(), m_markers.end(), Marker::compareByDrawOrder);
    return true;
}

bool MarkerManager::setPoint(MarkerID markerID, LngLat lngLat) {

    if (!m_scene) { return false; }

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

    if (!m_scene) { return false; }

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

    if (!m_scene) { return false; }

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

    if (!m_scene) { return false; }

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
            setVisible(marker->id(), marker->isVisible());
            rebuilt = true;
        }
    }
    m_zoom = zoom;
    return rebuilt;
}

void MarkerManager::removeAll() {

    m_markers.clear();

}

void MarkerManager::rebuildAll() {

    for (auto& entry : m_markers) {
        buildStyling(*entry);
        buildGeometry(*entry, m_zoom);
        setVisible(entry->id(), entry->isVisible());
    }

}

const std::vector<std::unique_ptr<Marker>>& MarkerManager::markers() const {
    return m_markers;
}

bool MarkerManager::buildStyling(Marker& marker) {

    if (!m_scene) { return false; }

    std::vector<StyleParam> params;

    const auto& markerStyling = marker.styling();

    // If the Marker styling is a path, find the layers it specifies.
    if (markerStyling.isPath) {
        auto path = markerStyling.string;
        // The DELIMITER used by layers is currently ":", but Marker paths use "." (scene.h).
        std::replace(path.begin(), path.end(), '.', DELIMITER[0]);
        // Start iterating over the delimited path components.
        size_t start = 0, end = 0;
        end = path.find(DELIMITER[0], start);
        if (path.compare(start, end - start, "layers") != 0) {
            // If the path doesn't begin with 'layers' it isn't a layer heirarchy.
            return false;
        }
        // Find the DataLayer named in our path.
        const SceneLayer* currentLayer = nullptr;
        size_t layerStart = end + 1;
        start = end + 1;
        end = path.find(DELIMITER[0], start);
        for (const auto& layer : m_scene->layers()) {
            if (path.compare(layerStart, end - layerStart, layer.name()) == 0) {
                currentLayer = &layer;
                marker.mergeRules(layer);
                break;
            }
        }
        // Search sublayers recursively until we can't find another token or layer.
        while (end != std::string::npos && currentLayer != nullptr) {
            start = end + 1;
            end = path.find(DELIMITER[0], start);
            const auto& layers = currentLayer->sublayers();
            currentLayer = nullptr;
            for (const auto& layer : layers) {
                if (path.compare(layerStart, end - layerStart, layer.name()) == 0) {
                    currentLayer = &layer;
                    marker.mergeRules(layer);
                    break;
                }
            }
        }
        // The last token found should have been "draw".
        if (path.compare(start, end - start, "draw") != 0) {
            return false;
        }
        // The draw group name should come next.
        start = end + 1;
        end = path.find(DELIMITER[0], start);
        // Find the rule in the merged set whose name matches the final token.
        return marker.finalizeRuleMergingForName(path.substr(start, end - start));
    }
    // If the styling is not a path, try to load it as a string of YAML.
    try {
        YAML::Node node = YAML::Load(markerStyling.string);
        // Parse style parameters from the YAML node.
        SceneLoader::parseStyleParams(node, m_scene, "", params);
    } catch (YAML::Exception e) {
        LOG("Invalid marker styling '%s', %s", markerStyling.string.c_str(), e.what());
        return false;
    }
    // Compile any new JS functions used for styling.
    const auto& sceneJsFnList = m_scene->functions();
    for (auto i = m_jsFnIndex; i < sceneJsFnList.size(); ++i) {
        m_styleContext.addFunction(sceneJsFnList[i]);
    }
    m_jsFnIndex = sceneJsFnList.size();

    marker.setDrawRuleData(std::make_unique<DrawRuleData>("", 0, std::move(params)));

    return true;
}

bool MarkerManager::buildGeometry(Marker& marker, int zoom) {

    auto feature = marker.feature();
    auto rule = marker.drawRule();
    if (!feature || !rule) { return false; }

    StyleBuilder* styler = nullptr;
    {
        auto name = rule->getStyleName();
        auto it = m_styleBuilders.find(name);
        if (it != m_styleBuilders.end()) {
            styler = it->second.get();
        } else {
            LOGN("Invalid style %s", name.c_str());
            return false;
        }
    }

    m_styleContext.setKeywordZoom(zoom);

    bool valid = marker.evaluateRuleForContext(m_styleContext);

    if (!valid) { return false; }

    styler->setup(marker, zoom);

    uint32_t selectionColor = 0;
    bool interactive = false;
    if (rule->get(StyleParamKey::interactive, interactive) && interactive) {
        if (selectionColor == 0) {
            selectionColor = m_scene->featureSelection()->nextColorIdentifier();
        }
        rule->selectionColor = selectionColor;
    } else {
        rule->selectionColor = 0;
    }

    if (!styler->addFeature(*feature, *rule)) { return false; }

    marker.setSelectionColor(selectionColor);
    marker.setMesh(styler->style().getID(), zoom, styler->build());

    return true;
}

const Marker* MarkerManager::getMarkerOrNullBySelectionColor(uint32_t selectionColor) const {
    for (const auto& marker : m_markers) {
        if (marker->isVisible() && marker->selectionColor() == selectionColor) {
            return marker.get();
        }
    }

    return nullptr;
}

Marker* MarkerManager::getMarkerOrNull(MarkerID markerID) {
    if (!markerID) { return nullptr; }
    for (const auto& entry : m_markers) {
        if (entry->id() == markerID) { return entry.get(); }
    }
    return nullptr;
}

} // namespace Tangram
