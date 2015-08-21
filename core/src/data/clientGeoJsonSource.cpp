#include "clientGeoJsonSource.h"
#include "platform.h"
#include "util/geom.h"

using namespace mapbox::util;

namespace Tangram {

const double extent = 4096;

Point transformPoint(geojsonvt::TilePoint pt) {
    return { 2 * pt.x / extent - 1, 1 - 2 * pt.y / extent, 0 };
}

ClientGeoJsonSource::ClientGeoJsonSource(const std::string& _name)
    : DataSource(_name, "") {}

void ClientGeoJsonSource::setData(const std::string& _data) {

    auto features = geojsonvt::GeoJSONVT::convertFeatures(_data);

    m_store = std::make_unique<GeoJSONVT>(features);

}

bool ClientGeoJsonSource::loadTileData(std::shared_ptr<TileTask>&& _task, TileTaskCb _cb) {

    _cb(std::move(_task));

    return true;
}

bool ClientGeoJsonSource::getTileData(std::shared_ptr<TileTask>& _task) {

    return true;
}

std::shared_ptr<TileData> ClientGeoJsonSource::parse(const Tile& _tile, std::vector<char>& _rawData) const {

    if (!m_store) { return nullptr; }

    auto data = std::make_shared<TileData>();

    auto id = _tile.getID();

    auto tile = m_store->getTile(id.z, id.x, id.y); // uses a mutex lock internally for thread-safety

    Layer layer(m_name);

    for (auto& it : tile.features) {

        Feature feat;

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
                    feat.polygons.back().push_back(line);
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
