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

bool ClientGeoJsonSource::process(const TileTask& _task,
                                  const MapProjection& _projection,
                                  TileDataSink& _sink) const {

    geojsonvt::Tile tile;
    {
        std::lock_guard<std::mutex> lock(m_mutexStore);
        if (!m_store) { return false; }
        tile = m_store->getTile(_task.tileId().z, _task.tileId().x, _task.tileId().y);
    }

    // empty name will skip filtering by 'collection'
    // TODO why not use DataSource name as layer name?
    if (!_sink.beginLayer("")) {
        return true;
    }

    Feature feat(m_id);

    for (auto& it : tile.features) {

        const auto& geom = it.tileGeometry;
        const auto type = it.type;

        feat.geometry.clear();

        switch (type) {
        case geojsonvt::TileFeatureType::Point:
            feat.geometry.type = GeometryType::points;
            break;
        case geojsonvt::TileFeatureType::LineString:
            feat.geometry.type = GeometryType::lines;
            break;
        case geojsonvt::TileFeatureType::Polygon:
            feat.geometry.type = GeometryType::polygons;
            break;
        default:
            continue;
        }

        feat.props = *it.tags.map;

        if (!_sink.matchFeature(feat)) { continue; }

        switch (type) {
            case geojsonvt::TileFeatureType::Point: {
                for (const auto& pt : geom) {
                    const auto& point = pt.get<geojsonvt::TilePoint>();
                    feat.geometry.addPoint(transformPoint(point));
                }
                break;
            }
            case geojsonvt::TileFeatureType::LineString: {
                for (const auto& r : geom) {
                    for (const auto& pt : r.get<geojsonvt::TileRing>().points) {
                        feat.geometry.addPoint(transformPoint(pt));
                    }
                    feat.geometry.endLine();
                }
                break;
            }
            case geojsonvt::TileFeatureType::Polygon: {
                for (const auto& r : geom) {
                    size_t begin = feat.geometry.points().size();

                    for (const auto& pt : r.get<geojsonvt::TileRing>().points) {
                        feat.geometry.addPoint(transformPoint(pt));
                    }
                    feat.geometry.endRing();

                    // Polygons are in a flat list of rings, with ccw rings indicating
                    // the beginning of a new polygon
                    if (feat.polygons().size() == 0 ||
                        signedArea(feat.geometry.points().begin() + begin,
                                   feat.geometry.points().end()) >= 0) {

                        feat.geometry.endPoly();
                    }
                }
                feat.geometry.endPoly();
                break;
            }
            default: break;
        }
        _sink.addFeature(feat);
    }

    return true;
}

}
