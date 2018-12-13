#include "marker/markerManager.h"

#include "data/tileData.h"
#include "gl/texture.h"
#include "marker/marker.h"
#include "scene/sceneLoader.h"
#include "scene/dataLayer.h"
#include "scene/styleContext.h"
#include "style/style.h"
#include "view/view.h"
#include "labels/labelSet.h"
#include "log.h"
#include "selection/featureSelection.h"

#include <algorithm>

namespace Tangram {

// ':' Delimiter for style params and layer-sublayer naming
static const char DELIMITER = ':';
MarkerManager::MarkerManager(Scene& _scene) : m_scene(_scene) {}

MarkerManager::~MarkerManager() {}


MarkerID MarkerManager::add() {
    m_dirty = true;

    // Add a new empty marker object to the list of markers.
    auto id = ++m_idCounter;
    m_markers.push_back(std::make_unique<Marker>(id));

    // Sort the marker list by draw order.
    std::stable_sort(m_markers.begin(), m_markers.end(), Marker::compareByDrawOrder);

    // Return a handle for the marker.
    return id;

}

bool MarkerManager::remove(MarkerID markerID) {
    m_dirty = true;

    for (auto it = m_markers.begin(), end = m_markers.end(); it != end; ++it) {
        if (it->get()->id() == markerID) {
            m_markers.erase(it);
            return true;
        }
    }
    return false;
}

bool MarkerManager::setStyling(MarkerID markerID, const char* styling, bool isPath) {
    if (!m_scene.isReady()) { return false; }

    Marker* marker = getMarkerOrNull(markerID);
    if (!marker) { return false; }

    m_dirty = true;

    marker->setStyling(std::string(styling), isPath);

    // Create a draw rule from the styling string.
    if (!buildStyling(*marker)) { return false; }

    // Build the feature mesh for the marker's current geometry.
    buildMesh(*marker, m_zoom);

    return true;
}

bool MarkerManager::setBitmap(MarkerID markerID, int width, int height, float density, const unsigned int* bitmapData) {
    if (!m_scene.isReady()) { return false; }

    Marker* marker = getMarkerOrNull(markerID);
    if (!marker) { return false; }

    m_dirty = true;

    TextureOptions options;
    options.displayScale = 1.f / density;
    auto texture = std::make_unique<Texture>(options);
    texture->setPixelData(width, height, sizeof(GLuint),
                          reinterpret_cast<const GLubyte*>(bitmapData),
                          width * height * sizeof(GLuint));

    marker->setTexture(std::move(texture));

    // The geometry is unchanged, but the mesh must be rebuilt because DynamicQuadMesh contains
    // texture batches as part of its data.
    buildMesh(*marker, m_zoom);

    return true;
}

bool MarkerManager::setVisible(MarkerID markerID, bool visible) {
    Marker* marker = getMarkerOrNull(markerID);
    if (!marker) { return false; }

    m_dirty = true;

    marker->setVisible(visible);
    return true;
}

bool MarkerManager::setDrawOrder(MarkerID markerID, int drawOrder) {
    Marker* marker = getMarkerOrNull(markerID);
    if (!marker) { return false; }

    m_dirty = true;

    marker->setDrawOrder(drawOrder);

    // Sort the marker list by draw order.
    std::stable_sort(m_markers.begin(), m_markers.end(), Marker::compareByDrawOrder);
    return true;
}

bool MarkerManager::setPoint(MarkerID markerID, LngLat lngLat) {
    if (!m_scene.isReady()) { return false; }

    Marker* marker = getMarkerOrNull(markerID);
    if (!marker) { return false; }

    m_dirty = true;

    marker->clearMesh();

    // If the marker does not have a 'point' feature mesh built, build it.
    if (!marker->mesh() || !marker->feature() || marker->feature()->geometryType != GeometryType::points) {
        auto feature = std::make_unique<Feature>();
        feature->geometryType = GeometryType::points;
        feature->points.emplace_back();
        marker->setFeature(std::move(feature));
        buildMesh(*marker, m_zoom);
    }

    // Update the marker's bounds to the given coordinates.
    auto origin = MapProjection::lngLatToProjectedMeters({lngLat.longitude, lngLat.latitude});
    marker->setBounds({ origin, origin });

    return true;
}

bool MarkerManager::setPointEased(MarkerID markerID, LngLat lngLat, float duration, EaseType ease) {
    if (!m_scene.isReady()) { return false; }

    Marker* marker = getMarkerOrNull(markerID);
    if (!marker) { return false; }

    m_dirty = true;

    // If the marker does not have a 'point' feature built, set that point immediately.
    if (!marker->mesh() || !marker->feature() || marker->feature()->geometryType != GeometryType::points) {
        return setPoint(markerID, lngLat);
    }

    auto dest = MapProjection::lngLatToProjectedMeters({lngLat.longitude, lngLat.latitude});
    marker->setEase(dest, duration, ease);

    return true;
}

bool MarkerManager::setPolyline(MarkerID markerID, LngLat* coordinates, int count) {
    if (!m_scene.isReady()) { return false; }

    Marker* marker = getMarkerOrNull(markerID);
    if (!marker) { return false; }

    m_dirty = true;

    marker->clearMesh();

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
    bounds.min = MapProjection::lngLatToProjectedMeters({bounds.min.x, bounds.min.y});
    bounds.max = MapProjection::lngLatToProjectedMeters({bounds.max.x, bounds.max.y});

    // Update the marker's bounds.
    marker->setBounds(bounds);

    float scale = 1.f / marker->modelScale();

    // Project and offset the coordinates into the marker-local coordinate system.
    auto origin = marker->origin(); // SW corner.
    for (int i = 0; i < count; ++i) {
        auto degrees = LngLat(coordinates[i].longitude, coordinates[i].latitude);
        auto meters = MapProjection::lngLatToProjectedMeters(degrees);
        line.emplace_back((meters.x - origin.x) * scale, (meters.y - origin.y) * scale);
    }

    // Update the feature data for the marker.
    marker->setFeature(std::move(feature));

    // Build a new mesh for the marker.
    buildMesh(*marker, m_zoom);

    return true;
}

bool MarkerManager::setPolygon(MarkerID markerID, LngLat* coordinates, int* counts, int rings) {
    if (!m_scene.isReady()) { return false; }

    Marker* marker = getMarkerOrNull(markerID);
    if (!marker) { return false; }

    m_dirty = true;

    marker->clearMesh();

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
    bounds.min = MapProjection::lngLatToProjectedMeters({bounds.min.x, bounds.min.y});
    bounds.max = MapProjection::lngLatToProjectedMeters({bounds.max.x, bounds.max.y});

    // Update the marker's bounds.
    marker->setBounds(bounds);

    float scale = 1.f / marker->modelScale();

    // Project and offset the coordinates into the marker-local coordinate system.
    auto origin = marker->origin(); // SW corner.
    ring = coordinates;
    for (int i = 0; i < rings; ++i) {
        int count = counts[i];
        polygon.emplace_back();
        auto& line = polygon.back();
        for (int j = 0; j < count; ++j) {
            auto degrees = LngLat(ring[j].longitude, ring[j].latitude);
            auto meters = MapProjection::lngLatToProjectedMeters(degrees);
            line.emplace_back((meters.x - origin.x) * scale, (meters.y - origin.y) * scale);
        }
        ring += count;
    }

    // Update the feature data for the marker.
    marker->setFeature(std::move(feature));

    // Build a new mesh for the marker.
    buildMesh(*marker, m_zoom);

    return true;
}

bool MarkerManager::update(const View& _view, float _dt) {
    if (!m_scene.isReady()) { return false; }

    if (!m_styleContext) {
        // First call to update after scene became ready
        //Initialize Stylecontext and StyleBuilders.
        m_styleContext = std::make_unique<StyleContext>();
        m_styleContext->initFunctions(m_scene);

        for (auto& style : m_scene.styles()) {
            m_styleBuilders[style->getName()] = style->createBuilder();
        }
    }

    m_zoom = _view.getZoom();

    bool rebuilt = false;
    bool easing = false;
    bool dirty = m_dirty;
    m_dirty = false;

    for (auto& marker : m_markers) {

        if (m_zoom != marker->builtZoomLevel()) {
            buildMesh(*marker, m_zoom);
            rebuilt = true;
        }

        marker->update(_dt, _view);
        easing |= marker->isEasing();
    }

    return rebuilt || easing || dirty;
}

void MarkerManager::removeAll() {
    m_dirty = true;

    m_markers.clear();

}

void MarkerManager::rebuildAll() {
    m_dirty = true;
    if (m_scene.isReady()) { return; }

    for (auto& entry : m_markers) {
        buildStyling(*entry);
        buildMesh(*entry, m_zoom);
    }
}

const std::vector<std::unique_ptr<Marker>>& MarkerManager::markers() const {
    return m_markers;
}

bool MarkerManager::buildStyling(Marker& marker) {
    if (!m_scene.isReady()) { return false; }

    const auto& markerStyling = marker.styling();

    // If the Marker styling is a path, find the layers it specifies.
    if (markerStyling.isPath) {
        auto path = markerStyling.string;
        // The DELIMITER used by layers is currently ":", but Marker paths use "." (scene.h).
        std::replace(path.begin(), path.end(), '.', DELIMITER);
        // Start iterating over the delimited path components.
        size_t start = 0, end = 0;
        end = path.find(DELIMITER, start);
        if (path.compare(start, end - start, "layers") != 0) {
            // If the path doesn't begin with 'layers' it isn't a layer heirarchy.
            return false;
        }
        // Find the DataLayer named in our path.
        const SceneLayer* currentLayer = nullptr;
        size_t layerStart = end + 1;
        start = end + 1;
        end = path.find(DELIMITER, start);
        for (const auto& layer : m_scene.layers()) {
            if (path.compare(layerStart, end - layerStart, layer.name()) == 0) {
                currentLayer = &layer;
                marker.mergeRules(layer);
                break;
            }
        }
        // Search sublayers recursively until we can't find another token or layer.
        while (end != std::string::npos && currentLayer != nullptr) {
            start = end + 1;
            end = path.find(DELIMITER, start);
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
        end = path.find(DELIMITER, start);
        // Find the rule in the merged set whose name matches the final token.
        return marker.finalizeRuleMergingForName(path.substr(start, end - start));
    }

    std::vector<StyleParam> params;

    // If the styling is not a path, try to load it as a string of YAML.
    size_t start = m_functions.size();

    try {
        YAML::Node node = YAML::Load(markerStyling.string);
        SceneLoader::parseStyleParams(m_stops, m_functions, node, "", params);
    } catch (const YAML::Exception& e) {
        LOG("Invalid marker styling '%s', %s", markerStyling.string.c_str(), e.what());
        return false;
    }

    size_t offset = start + m_scene.functions().size();
    for (auto& p : params) {
        if (p.function >= 0) { p.function += offset; }
    }
    // Compile any new JS functions used for styling.
    for (auto i = start; i < m_functions.size(); ++i) {
        m_styleContext->addFunction(m_functions[i]);
    }

    marker.setDrawRuleData(std::make_unique<DrawRuleData>("", 0, std::move(params)));

    return true;
}

bool MarkerManager::buildMesh(Marker& marker, int zoom) {

    marker.clearMesh();

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

    // Apply defaul draw rules defined for this style
    styler->style().applyDefaultDrawRules(*rule);

    m_styleContext->setKeywordZoom(zoom);

    bool valid = marker.evaluateRuleForContext(*m_styleContext);

    if (!valid) { return false; }

    styler->setup(marker, zoom);

    uint32_t selectionColor = 0;
    bool interactive = false;
    if (rule->get(StyleParamKey::interactive, interactive) && interactive) {
        if (selectionColor == 0) {
            selectionColor = m_scene.featureSelection()->nextColorIdentifier();
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
