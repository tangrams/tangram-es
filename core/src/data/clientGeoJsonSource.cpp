#include "clientGeoJsonSource.h"
#define GEOJSONVT_CUSTOM_TAGS
#include "mapbox/geojsonvt/geojsonvt_types.hpp"
#include "mapbox/geojsonvt/geojsonvt.hpp"
#include "mapbox/geojsonvt/geojsonvt_convert.hpp"
#include "platform.h"
#include "tangram.h"
#include "tile/tileTask.h"
#include "util/geom.h"
#include "data/propertyItem.h"
#include "data/tileData.h"
#include "tile/tile.h"
#include "view/view.h"

#include <regex>

using namespace mapbox::util;

namespace Tangram {

const double extent = 4096;
const uint32_t indexMaxPoints = 100000;
double tolerance = 1E-8;

std::shared_ptr<TileTask> ClientGeoJsonSource::createTask(TileID _tileId, int _subTask) {
    return std::make_shared<TileTask>(_tileId, shared_from_this(), _subTask);
}

// Transform a geojsonvt::TilePoint into the corresponding Tangram::Point
Point transformPoint(geojsonvt::TilePoint pt) {
    return { pt.x / extent, 1. - pt.y / extent, 0 };
}

// TODO: pass scene's resourcePath to constructor to be used with `stringFromFile`
ClientGeoJsonSource::ClientGeoJsonSource(const std::string& _name, const std::string& _url,
                                         int32_t _minDisplayZoom, int32_t _maxDisplayZoom, int32_t _maxZoom)
    : DataSource(_name, _url, _minDisplayZoom, _maxDisplayZoom, _maxZoom) {

    // TODO: handle network url for client datasource data
    // TODO: generic uri handling
    m_generateGeometry = true;

    if (!_url.empty()) {
        std::regex r("^(http|https):/");
        std::smatch match;
        if (std::regex_search(_url, match, r)) {
            startUrlRequest(_url,
                    [&, this](std::vector<char>&& rawData) {
                        addData(std::string(rawData.begin(), rawData.end()));
                        m_hasPendingData = false;
                    });
            m_hasPendingData = true;
        } else {
            // Load from file
            const auto& string = stringFromFile(_url.c_str());
            addData(string);
        }
    }
}

ClientGeoJsonSource::~ClientGeoJsonSource() {}

void ClientGeoJsonSource::addData(const std::string& _data) {

    auto features = geojsonvt::GeoJSONVT::convertFeatures(_data);

    for (auto& f : features) {
        m_features.push_back(std::move(f));
    }

    std::lock_guard<std::mutex> lock(m_mutexStore);
    m_store = std::make_unique<GeoJSONVT>(m_features, m_maxZoom, m_maxZoom, indexMaxPoints, tolerance);
    m_generation++;

}

bool ClientGeoJsonSource::loadTileData(std::shared_ptr<TileTask>&& _task, TileTaskCb _cb) {

    if (m_hasPendingData) {
        return false;
    }

    _cb.func(std::move(_task));

    return true;
}

void ClientGeoJsonSource::clearData() {

    m_features.clear();

    std::lock_guard<std::mutex> lock(m_mutexStore);
    m_store.reset();
    m_generation++;
}

void ClientGeoJsonSource::addPoint(const Properties& _tags, LngLat _point) {

    auto container = geojsonvt::Convert::project({ geojsonvt::LonLat(_point.longitude, _point.latitude) }, tolerance);

    geojsonvt::Tags tags;

    auto feature = geojsonvt::Convert::create(geojsonvt::Tags{std::make_shared<Properties>(_tags)},
                                              geojsonvt::ProjectedFeatureType::Point,
                                              container.members);

    m_features.push_back(std::move(feature));

    std::lock_guard<std::mutex> lock(m_mutexStore);
    m_store = std::make_unique<GeoJSONVT>(m_features, m_maxZoom, m_maxZoom, indexMaxPoints, tolerance);
    m_generation++;
}

void ClientGeoJsonSource::addLine(const Properties& _tags, const Coordinates& _line) {
    auto& line = reinterpret_cast<const std::vector<geojsonvt::LonLat>&>(_line);

    std::vector<geojsonvt::ProjectedGeometry> geometry = { geojsonvt::Convert::project(line, tolerance) };

    auto feature = geojsonvt::Convert::create(geojsonvt::Tags{std::make_shared<Properties>(_tags)},
                                              geojsonvt::ProjectedFeatureType::LineString,
                                              geometry);

    m_features.push_back(std::move(feature));

    std::lock_guard<std::mutex> lock(m_mutexStore);
    m_store = std::make_unique<GeoJSONVT>(m_features, m_maxZoom, m_maxZoom, indexMaxPoints, tolerance);
    m_generation++;
}

void ClientGeoJsonSource::addPoly(const Properties& _tags, const std::vector<Coordinates>& _poly) {

    geojsonvt::ProjectedGeometryContainer geometry;
    for (auto& _ring : _poly) {
        auto& ring = reinterpret_cast<const std::vector<geojsonvt::LonLat>&>(_ring);
        geometry.members.push_back(geojsonvt::Convert::project(ring, tolerance));
    }

    auto feature = geojsonvt::Convert::create(geojsonvt::Tags{std::make_shared<Properties>(_tags)},
                                              geojsonvt::ProjectedFeatureType::Polygon,
                                              geometry);

    m_features.push_back(std::move(feature));

    std::lock_guard<std::mutex> lock(m_mutexStore);
    m_store = std::make_unique<GeoJSONVT>(m_features, m_maxZoom, m_maxZoom, indexMaxPoints, tolerance);
    m_generation++;
}

std::shared_ptr<TileData> ClientGeoJsonSource::parse(const TileTask& _task,
                                                     const MapProjection& _projection) const {

    auto data = std::make_shared<TileData>();

    geojsonvt::Tile tile;
    {
        std::lock_guard<std::mutex> lock(m_mutexStore);
        if (!m_store) { return nullptr; }
        tile = m_store->getTile(_task.tileId().z, _task.tileId().x, _task.tileId().y);
    }

    Layer layer(""); // empty name will skip filtering by 'collection'

    for (auto& it : tile.features) {

        Feature feat(m_id);

        const auto& geom = it.tileGeometry;
        const auto type = it.type;

        switch (type) {
            case geojsonvt::TileFeatureType::Point: {
                feat.geometryType = GeometryType::points;
                for (const auto& pt : geom) {
                    const auto& point = pt.get<geojsonvt::TilePoint>();
                    feat.points.push_back(transformPoint(point));
                }
                break;
            }
            case geojsonvt::TileFeatureType::LineString: {
                feat.geometryType = GeometryType::lines;
                for (const auto& r : geom) {
                    Line line;
                    for (const auto& pt : r.get<geojsonvt::TileRing>().points) {
                        line.push_back(transformPoint(pt));
                    }
                    feat.lines.emplace_back(std::move(line));
                }
                break;
            }
            case geojsonvt::TileFeatureType::Polygon: {
                feat.geometryType = GeometryType::polygons;
                for (const auto& r : geom) {
                    Line line;
                    for (const auto& pt : r.get<geojsonvt::TileRing>().points) {
                        line.push_back(transformPoint(pt));
                    }
                    // Polygons are in a flat list of rings, with ccw rings indicating
                    // the beginning of a new polygon
                    if (signedArea(line.begin(), line.end()) >= 0 || feat.polygons.empty()) {
                        feat.polygons.emplace_back();
                    }
                    feat.polygons.back().push_back(std::move(line));
                }
                break;
            }
            default: break;
        }

        feat.props = *it.tags.map;
        layer.features.emplace_back(std::move(feat));

    }

    data->layers.emplace_back(std::move(layer));

    return data;

}

}
