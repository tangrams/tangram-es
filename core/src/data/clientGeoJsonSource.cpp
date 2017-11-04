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
#include <log.h>

namespace Tangram {

using namespace mapbox;

geojsonvt::Options options() {
    geojsonvt::Options opt;
    opt.maxZoom = 18;
    opt.indexMaxZoom = 5;
    opt.indexMaxPoints = 100000;
    opt.solidChildren = true;
    opt.tolerance = 3;
    opt.extent = 4096;
    opt.buffer = 0;
    return opt;
}

struct ClientGeoJsonData {
    std::unique_ptr<geojsonvt::GeoJSONVT> tiles;
    mapbox::geometry::feature_collection<double> features;
    std::vector<Properties> properties;
    std::map<uint64_t, uint64_t> polylineIds;
};

std::shared_ptr<TileTask> ClientGeoJsonSource::createTask(TileID _tileId, int _subTask) {
    return std::make_shared<TileTask>(_tileId, shared_from_this(), _subTask);
}


// TODO: pass scene's resourcePath to constructor to be used with `stringFromFile`
ClientGeoJsonSource::ClientGeoJsonSource(std::shared_ptr<Platform> _platform,
                                         const std::string& _name, const std::string& _url,
                                         bool _generateCentroids,
                                         TileSource::ZoomOptions _zoomOptions)

    : TileSource(_name, nullptr, _zoomOptions),
      m_generateCentroids(_generateCentroids),
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

struct add_centroid {

    geometry::point<double>& pt;

    bool operator()(const geometry::polygon<double>& geom) {
        if (geom.empty()) { return false; }
        pt = centroid(geom.front().begin(), geom.front().end()-1);
        if (std::isnan(pt.x) || std::isnan(pt.y)) {
            return false;
        }
        return true;
    }

    bool operator()(const geometry::multi_polygon<double>& geom) {
        float largestArea = 0.f;
        size_t largestAreaIndex = 0;
        for (size_t index = 0; index < geom.size(); index++) {
            auto& g = geom[index];
            if (g.empty()){
                continue;
            }
            auto area = signedArea(g.front().begin(), g.front().end());
            if (area > largestArea) {
                largestArea = area;
                largestAreaIndex = index;
            }
        }
        return (*this)(geom[largestAreaIndex]);
    }

    template <typename T>
    bool operator()(T) {
        // Ignore all other geometry types
        return false;
    }
};

struct prop_visitor {
    Properties& props;
    std::string& key;
    void operator()(std::string v) {
        props.set(key, v);
    }
    void operator()(bool v) {
        props.set(key, double(v));
    }
    void operator()(uint64_t v) {
        props.set(key, double(v));
    }
    void operator()(int64_t v) {
        props.set(key, double(v));
    }
    void operator()(double v) {
        props.set(key, v);
    }

    template <typename T>
    void operator()(T) {
        // Ignore all other basic value types
    }
};

void ClientGeoJsonSource::generateLabelCentroidFeature() {
    for (const auto &feat : m_store->features) {
        geometry::point<double> centroid;
        const auto& properties = m_store->properties[feat.id.get<uint64_t>()];
        if (geometry::geometry<double>::visit(feat.geometry, add_centroid{ centroid })) {
            uint64_t id = m_store->features.size();
            m_store->features.emplace_back(centroid, id);
            m_store->properties.push_back(properties);
            auto& props = m_store->properties.back();
            props.set("label_placement", 1.0);
        }
    }
}

void ClientGeoJsonSource::addData(const std::string& _data) {

    std::lock_guard<std::mutex> lock(m_mutexStore);

    const auto json = geojson::parse(_data);
    auto features = geojsonvt::geojson::visit(json, geojsonvt::ToFeatureCollection{});

    for (auto& feature : features) {

        feature.id = uint64_t(m_store->properties.size());
        m_store->properties.emplace_back();
        Properties& props = m_store->properties.back();

        for (const auto& prop : feature.properties) {
            auto key = prop.first;
            prop_visitor visitor = {props, key};
            mapbox::util::apply_visitor(visitor, prop.second);
        }
        feature.properties.clear();
    }

    m_store->features.insert(m_store->features.end(),
                             std::make_move_iterator(features.begin()),
                             std::make_move_iterator(features.end()));

    if (m_generateCentroids) {
        generateLabelCentroidFeature();
    }

    m_store->tiles = std::make_unique<geojsonvt::GeoJSONVT>(m_store->features, options());
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

uint64_t ClientGeoJsonSource::addPoint(Properties& _tags, LngLat _point) {

    std::lock_guard<std::mutex> lock(m_mutexStore);

    geometry::point<double> geom { _point.longitude, _point.latitude };

    uint64_t i = m_store->features.size();

    m_store->features.emplace_back(geom, i);
    m_store->properties.emplace_back(_tags);

    m_store->tiles = std::make_unique<geojsonvt::GeoJSONVT>(m_store->features, options());
    m_generation++;

    return i;
}

uint64_t ClientGeoJsonSource::addLine(Properties& _tags, const Coordinates& _line) {

    std::lock_guard<std::mutex> lock(m_mutexStore);

    geometry::line_string<double> geom;
    for (auto& p : _line) {
        geom.emplace_back(p.longitude, p.latitude);
    }

    uint64_t i = m_store->features.size();
    uint64_t id = m_store->polylineIds.size();

    _tags.set("id", std::to_string(id));

    m_store->features.emplace_back(geom, i);
    m_store->properties.emplace_back(_tags);
    m_store->polylineIds.insert(std::pair<uint64_t, uint64_t>(id, i));

    m_store->tiles = std::make_unique<geojsonvt::GeoJSONVT>(m_store->features, options());
    m_generation++;

    return id;
}

uint64_t ClientGeoJsonSource::addPoly(Properties& _tags, const std::vector<Coordinates>& _poly) {

    std::lock_guard<std::mutex> lock(m_mutexStore);

    geometry::polygon<double> geom;
    for (auto& ring : _poly) {
        geom.emplace_back();
        auto &line = geom.back();
        for (auto& p : ring) {
            line.emplace_back(p.longitude, p.latitude);
        }
    }

    uint64_t i = m_store->features.size();

    m_store->features.emplace_back(geom, i);
    m_store->properties.emplace_back(_tags);

    if (m_generateCentroids) {
        generateLabelCentroidFeature();
    }

    m_store->tiles = std::make_unique<geojsonvt::GeoJSONVT>(m_store->features, options());
    m_generation++;

    return i;
}

void ClientGeoJsonSource::updateLine(uint64_t id, const Coordinates& _line) {

    std::lock_guard<std::mutex> lock(m_mutexStore);

    std::map<uint64_t, uint64_t>::iterator findIt = m_store->polylineIds.find(id);

    if (findIt != m_store->polylineIds.end() && findIt->second < m_store->features.size()) {
        uint64_t i = findIt->second;

        geometry::line_string<double> geom;
        for (auto &p : _line) {
            geom.emplace_back(p.longitude, p.latitude);
        }

        m_store->features[i] = mapbox::geometry::feature<double>(geom, i);

        m_store->tiles = std::make_unique<geojsonvt::GeoJSONVT>(m_store->features, options());
        m_generation++;
    }
}

void ClientGeoJsonSource::updateLine(uint64_t id, const Properties& _tags) {

    std::lock_guard<std::mutex> lock(m_mutexStore);

    std::map<uint64_t, uint64_t>::iterator findIt = m_store->polylineIds.find(id);

    if (findIt != m_store->polylineIds.end() && findIt->second < m_store->properties.size()) {
        uint64_t i = findIt->second;

        for (int j = 0; j < _tags.items().size(); ++j) {
            m_store->properties[i].set(_tags.items()[j].key, _tags.getString(_tags.items()[j].key));
        }

        m_store->tiles = std::make_unique<geojsonvt::GeoJSONVT>(m_store->features, options());
        m_generation++;
    }
}

void ClientGeoJsonSource::removeLine(uint64_t id) {

    std::lock_guard<std::mutex> lock(m_mutexStore);

    std::map<uint64_t, uint64_t>::iterator findIt = m_store->polylineIds.find(id);

    if (findIt != m_store->polylineIds.end() && findIt->second < m_store->features.size()) {
        uint64_t i = findIt->second;

        m_store->features.erase(std::next(m_store->features.begin(), i));
        m_store->properties.erase(std::next(m_store->properties.begin(), i));

        for (std::map<uint64_t, uint64_t>::iterator it = m_store->polylineIds.begin(); it != m_store->polylineIds.end(); ++it) {
            if (it->second > i) {
                it->second = it->second - 1;
                m_store->features[it->second].id = it->second;
            }
        }

        m_store->tiles = std::make_unique<geojsonvt::GeoJSONVT>(m_store->features, options());
        m_generation++;
    }
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
            auto tp = transformPoint(p);
            if (line.size() > 0 && tp == line.back()) { continue; }
            line.push_back(tp);
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
                auto tp = transformPoint(p);
                if (line.size() > 0 && tp == line.back()) { continue; }
                line.push_back(tp);
            }
        }
        return true;
    }

    bool operator()(const geometry::multi_point<int16_t>& geom) {
        for (auto& g : geom) { (*this)(g); }
        return true;
    }

    bool operator()(const geometry::multi_line_string<int16_t>& geom) {
        for (auto& g : geom) { (*this)(g); }
        return true;
    }

    bool operator()(const geometry::multi_polygon<int16_t>& geom) {
        for (auto& g : geom) { (*this)(g); }
        return true;
    }

    template <typename T>
    bool operator()(T) {
        // Ignore GeometryCollection
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
