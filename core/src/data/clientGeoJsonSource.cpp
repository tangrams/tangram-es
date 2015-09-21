#include "clientGeoJsonSource.h"
#include "mapbox/geojsonvt/geojsonvt.hpp"
#include "mapbox/geojsonvt/geojsonvt_convert.hpp"
#include "mapbox/geojsonvt/geojsonvt_types.hpp"
#include "platform.h"
#include "util/geom.h"
#include "data/propertyItem.h"

using namespace mapbox::util;

namespace Tangram {

const double extent = 4096;
const uint8_t maxZoom = 18;
const uint8_t indexMaxZoom = 18;
const uint32_t indexMaxPoints = 100000;
double tolerance = 1E-8;

Point ClientGeoJsonSource::transformPoint(geojsonvt::TilePoint pt) const {
    return { pt.x / extent, 1. - pt.y / extent, 0 };
}

ClientGeoJsonSource::ClientGeoJsonSource(const std::string& _name, const std::string& _url)
    : DataSource(_name, _url) {

    if (!_url.empty()) {
        // Load from file
        const auto& string = stringFromFile(_url.c_str(), PathType::resource);
        addData(string);
    }
}

ClientGeoJsonSource::~ClientGeoJsonSource() {}

void ClientGeoJsonSource::addData(const std::string& _data) {

    m_features = geojsonvt::GeoJSONVT::convertFeatures(_data);
    m_store = std::make_unique<GeoJSONVT>(m_features, maxZoom, indexMaxZoom, indexMaxPoints, tolerance);

}

bool ClientGeoJsonSource::loadTileData(std::shared_ptr<TileTask>&& _task, TileTaskCb _cb) {

    _cb(std::move(_task));

    return true;
}

bool ClientGeoJsonSource::getTileData(std::shared_ptr<TileTask>& _task) {

    return true;
}

void ClientGeoJsonSource::clearData() {

    m_features.clear();
    m_store.reset();

}

void ClientGeoJsonSource::addPoint(const Tags& _tags, LngLat _point) {

    auto container = geojsonvt::Convert::project({ geojsonvt::LonLat(_point.longitude, _point.latitude) }, tolerance);

    auto feature = geojsonvt::Convert::create(_tags,
                                              geojsonvt::ProjectedFeatureType::Point,
                                              container.members);

    m_features.push_back(std::move(feature));
    m_store = std::make_unique<GeoJSONVT>(m_features, maxZoom, indexMaxZoom, indexMaxPoints, tolerance);

}

void ClientGeoJsonSource::addLine(const Tags& _tags, const Coordinates& _line) {
    auto& line = reinterpret_cast<const std::vector<geojsonvt::LonLat>&>(_line);
    
    std::vector<geojsonvt::ProjectedGeometry> geometry = { geojsonvt::Convert::project(line, tolerance) };

    auto feature = geojsonvt::Convert::create(_tags,
                                              geojsonvt::ProjectedFeatureType::LineString,
                                              geometry);
    
    m_features.push_back(std::move(feature));
    m_store = std::make_unique<GeoJSONVT>(m_features, maxZoom, indexMaxZoom, indexMaxPoints, tolerance);
}

void ClientGeoJsonSource::addPoly(const Tags& _tags, const std::vector<Coordinates>& _poly) {

    geojsonvt::ProjectedGeometryContainer geometry;
    for (auto& _ring : _poly) {
        auto& ring = reinterpret_cast<const std::vector<geojsonvt::LonLat>&>(_ring);
        geometry.members.push_back(geojsonvt::Convert::project(ring, tolerance));
    }

    auto feature = geojsonvt::Convert::create(_tags,
                                              geojsonvt::ProjectedFeatureType::Polygon,
                                              geometry);

    m_features.push_back(std::move(feature));
    m_store = std::make_unique<GeoJSONVT>(m_features, maxZoom, indexMaxZoom, indexMaxPoints, tolerance);
}

std::shared_ptr<TileData> ClientGeoJsonSource::parse(const Tile& _tile, std::vector<char>& _rawData) const {

    if (!m_store) { return nullptr; }

    auto data = std::make_shared<TileData>();

    auto id = _tile.getID();

    auto tile = m_store->getTile(id.z, id.x, id.y); // uses a mutex lock internally for thread-safety

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
                    if (signedArea(line) >= 0 || feat.polygons.empty()) {
                        feat.polygons.emplace_back();
                    }
                    feat.polygons.back().push_back(std::move(line));
                }
                break;
            }
            default: break;
        }

        std::vector<Properties::Item> items;
        items.reserve(it.tags.size());

        for (auto& tag : it.tags) {
            items.emplace_back(tag.first, tag.second);
        }

        feat.props = Properties(std::move(items));

        layer.features.emplace_back(std::move(feat));

    }

    data->layers.emplace_back(std::move(layer));

    return data;
    
}

}
