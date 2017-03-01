#include "data/clientGeoJsonSource.h"

#include "platform.h"
#include "tile/tileTask.h"
#include "util/geom.h"
#include "data/propertyItem.h"
#include "data/tileData.h"
#include "tile/tile.h"
#include "view/view.h"

#include "mapbox/geojsonvt.hpp"

// RapidJson parser
#include "mapbox/geojson.hpp"
#include <mapbox/geojson_impl.hpp>


#include <regex>

namespace Tangram {

using namespace mapbox;

struct ClientGeoJsonData {
    std::unique_ptr<geojsonvt::GeoJSONVT> tiles;
    mapbox::geometry::feature_collection<double> features;
    std::vector<Properties> properties;
};

std::shared_ptr<TileTask> ClientGeoJsonSource::createTask(TileID _tileId, int _subTask) {
    return std::make_shared<TileTask>(_tileId, shared_from_this(), _subTask);
}


// TODO: pass scene's resourcePath to constructor to be used with `stringFromFile`
ClientGeoJsonSource::ClientGeoJsonSource(std::shared_ptr<Platform> _platform,
                                         const std::string& _name, const std::string& _url,
                                         int32_t _minDisplayZoom, int32_t _maxDisplayZoom, int32_t _maxZoom)

    : TileSource(_name, nullptr, _minDisplayZoom, _maxDisplayZoom, _maxZoom),
      m_platform(_platform) {

    // TODO: handle network url for client datasource data
    // TODO: generic uri handling
    m_generateGeometry = true;
    m_store = std::make_unique<ClientGeoJsonData>();

    if (!_url.empty()) {
        std::regex r("^(http|https):/");
        std::smatch match;
        if (std::regex_search(_url, match, r)) {
            m_platform->startUrlRequest(_url,
                    [&, this](std::vector<char>&& rawData) {
                        addData(std::string(rawData.begin(), rawData.end()));
                        m_hasPendingData = false;
                    });
            m_hasPendingData = true;
        } else {
            // Load from file
            addData(m_platform->stringFromFile(_url.c_str()));
        }
    }
}

ClientGeoJsonSource::~ClientGeoJsonSource() {}

void ClientGeoJsonSource::addData(const std::string& _data) {

    std::lock_guard<std::mutex> lock(m_mutexStore);

    const auto json = geojson::parse(_data);
    auto features = geojsonvt::geojson::visit(json, geojsonvt::ToFeatureCollection{});

    for (auto& feature : features) {

        feature.id = uint64_t(m_store->properties.size());
        m_store->properties.emplace_back();
        Properties& props = m_store->properties.back();

        for (const auto& prop : feature.properties) {

            if (prop.second.is<std::string>()) {
                props.set(prop.first, prop.second.get<std::string>());

            } else if (prop.second.is<bool>()) {
                props.set(prop.first, double(prop.second.get<bool>()));

            } else if (prop.second.is<uint64_t>()) {
                props.set(prop.first, double(prop.second.get<uint64_t>()));

            } else if (prop.second.is<int64_t>()) {
                props.set(prop.first, double(prop.second.get<int64_t>()));

            } else if (prop.second.is<double>()) {
                props.set(prop.first, prop.second.get<double>());
            }
        }
        feature.properties.clear();
    }

    m_store->features.insert(m_store->features.end(),
                             std::make_move_iterator(features.begin()),
                             std::make_move_iterator(features.end()));

    m_store->tiles = std::make_unique<geojsonvt::GeoJSONVT>(m_store->features);
    m_generation++;
}

void ClientGeoJsonSource::loadTileData(std::shared_ptr<TileTask> _task, TileTaskCb _cb) {

    if (m_hasPendingData) {
        return;
    }

    if (_task->needsLoading()) {
        _task->startedLoading();

        _cb.func(_task);
    }

    // Load subsources
    TileSource::loadTileData(_task, _cb);
}

void ClientGeoJsonSource::clearData() {

    std::lock_guard<std::mutex> lock(m_mutexStore);

    m_store->features.clear();
    m_store->properties.clear();
    m_store->tiles.reset();

    m_generation++;
}

void ClientGeoJsonSource::addPoint(const Properties& _tags, LngLat _point) {

    std::lock_guard<std::mutex> lock(m_mutexStore);

    geometry::point<double> geom { _point.longitude, _point.latitude };

    uint64_t id = m_store->features.size();

    m_store->features.emplace_back(geom, id);
    m_store->properties.emplace_back(_tags);

    m_store->tiles = std::make_unique<geojsonvt::GeoJSONVT>(m_store->features);
    m_generation++;
}

void ClientGeoJsonSource::addLine(const Properties& _tags, const Coordinates& _line) {


    std::lock_guard<std::mutex> lock(m_mutexStore);

    geometry::line_string<double> geom;
    for (auto& p : _line) {
        geom.emplace_back(p.longitude, p.latitude);
    }

    uint64_t id = m_store->features.size();

    m_store->features.emplace_back(geom, id);
    m_store->properties.emplace_back(_tags);

    m_store->tiles = std::make_unique<geojsonvt::GeoJSONVT>(m_store->features);
    m_generation++;
}

void ClientGeoJsonSource::addPoly(const Properties& _tags, const std::vector<Coordinates>& _poly) {


    std::lock_guard<std::mutex> lock(m_mutexStore);

    geometry::polygon<double> geom;
    for (auto& ring : _poly) {
        geom.emplace_back();
        auto &line = geom.back();
        for (auto& p : ring) {
            line.emplace_back(p.longitude, p.latitude);
        }
    }

    uint64_t id = m_store->features.size();

    m_store->features.emplace_back(geom, id);
    m_store->properties.emplace_back(_tags);

    m_store->tiles = std::make_unique<geojsonvt::GeoJSONVT>(m_store->features);
    m_generation++;
}

struct add_geometry {

    static constexpr double extent = 4096.0;

    // Transform a geojsonvt::TilePoint into the corresponding Tangram::Point
    Point transformPoint(geometry::point<int16_t> pt) {
        return { pt.x / extent, 1. - pt.y / extent, 0 };
    }

    Feature& feature;

    bool operator()(const geometry::point<int16_t>& p) {
        feature.geometryType = GeometryType::points;
        feature.points.push_back(transformPoint(p));
        return true;
    }
    bool operator()(const geometry::line_string<int16_t>& geom) {
        feature.geometryType = GeometryType::lines;
        feature.lines.emplace_back();
        Line& line = feature.lines.back();
        for (const auto& p : geom) {
            line.push_back(transformPoint(p));
        }
        return true;
    }
    bool operator()(const geometry::polygon<int16_t>& geom) {
        feature.geometryType = GeometryType::polygons;
        feature.polygons.emplace_back();
        for (const auto& ring : geom) {
            feature.polygons.back().emplace_back();
            Line& line = feature.polygons.back().back();
            for (const auto& p : ring) {
                line.push_back(transformPoint(p));
            }
        }
        return true;
    }

    template <typename T>
    bool operator()(T) {
        // Unreachable: All multi-geometries and feature collections
        // are split up in vector tiles.
        return false;
    }
};

std::shared_ptr<TileData> ClientGeoJsonSource::parse(const TileTask& _task,
                                                     const MapProjection& _projection) const {

    std::lock_guard<std::mutex> lock(m_mutexStore);

    auto data = std::make_shared<TileData>();

    if (!m_store->tiles) { return nullptr; }
    auto tile = m_store->tiles->getTile(_task.tileId().z, _task.tileId().x, _task.tileId().y);

    data->layers.emplace_back("");  // empty name will skip filtering by 'collection'
    Layer& layer = data->layers.back();

    for (auto& it : tile.features) {
        Feature feature(m_id);

        if (geometry::geometry<int16_t>::visit(it.geometry, add_geometry{ feature })) {
            feature.props = m_store->properties[it.id.get<uint64_t>()];
            layer.features.emplace_back(std::move(feature));
        }
    }


    return data;
}

}
