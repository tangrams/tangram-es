#include "clientGeoJsonSource.h"
#include "platform.h"

using namespace mapbox::util;

namespace Tangram {

const double extent = 4096;

ClientGeoJsonSource::ClientGeoJsonSource(const std::string& _name)
    : DataSource(_name, "") {}

void ClientGeoJsonSource::setData(const std::string& _data) {

    auto features = geojsonvt::GeoJSONVT::convertFeatures(_data);

    m_store = std::make_unique<GeoJSONVT>(features);

}

bool ClientGeoJsonSource::loadTileData(std::shared_ptr<TileTask>&& _task, TileTaskCb _cb) {

    _cb(std::move(_task));

    requestRender();

    return true;
}

bool ClientGeoJsonSource::getTileData(std::shared_ptr<TileTask>& _task) {

    return true;
}

std::shared_ptr<TileData> ClientGeoJsonSource::parse(const Tile& _tile, std::vector<char>& _rawData) const {

    if (!m_store) { return nullptr; }

    auto data = std::make_shared<TileData>();

    auto id = _tile.getID();

    auto tile = m_store->getTile(id.z, id.x, id.y);

    Layer layer(m_name);

    for (auto& it : tile.features) {

        Feature feat;

        const auto& geom = it.tileGeometry;
        const auto type = it.type;

        switch (type) {
            case geojsonvt::TileFeatureType::Point:      feat.geometryType = GeometryType::points; break;
            case geojsonvt::TileFeatureType::LineString: feat.geometryType = GeometryType::lines; break;
            case geojsonvt::TileFeatureType::Polygon:    feat.geometryType = GeometryType::polygons; break;
            default: break;
        }

        if (type == geojsonvt::TileFeatureType::Point) {
            for (const auto& pt : geom) {
                const auto& point = pt.get<geojsonvt::TilePoint>();
                feat.points.push_back({ point.x / extent, point.y / extent, 0 });

            }
        } else {
            for (const auto& r : geom) {
                Line line;
                for (const auto& pt : r.get<geojsonvt::TileRing>().points) {
                    line.push_back({ pt.x / extent, pt.y / extent, 0 });
                }
                feat.lines.emplace_back(std::move(line));
            }
        }

        for (auto& tag : it.tags) {
            feat.props.stringProps[tag.first] = tag.second;
        }

        layer.features.emplace_back(std::move(feat));

    }

    data->layers.emplace_back(std::move(layer));

    return data;
    
}

}
