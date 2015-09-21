#pragma once

#include "dataSource.h"
#include "util/types.h"

namespace mapbox {
namespace util {
namespace geojsonvt {
class GeoJSONVT;
class TilePoint;
class ProjectedFeature;
}}}

namespace Tangram {

using GeoJSONVT = mapbox::util::geojsonvt::GeoJSONVT;
using Tags = std::map<std::string, std::string>;

class ClientGeoJsonSource : public DataSource {

public:

    ClientGeoJsonSource(const std::string& _name, const std::string& _url);
    ~ClientGeoJsonSource();

    // Add geometry from a GeoJSON string
    void addData(const std::string& _data);
    void addPoint(const Tags& _tags, LngLat _point);
    void addLine(const Tags& _tags, const Coordinates& _line);
    void addPoly(const Tags& _tags, const std::vector<Coordinates>& _poly);

    virtual bool loadTileData(std::shared_ptr<TileTask>&& _task, TileTaskCb _cb) override;
    virtual bool getTileData(std::shared_ptr<TileTask>& _task) override;
    virtual void cancelLoadingTile(const TileID& _tile) override {};
    virtual void clearData() override;

protected:

    virtual std::shared_ptr<TileData> parse(const Tile& _tile, std::vector<char>& _rawData) const override;

    // Transform a geojsonvt::TilePoint into the corresponding Tangram::Point
    Point transformPoint(mapbox::util::geojsonvt::TilePoint pt) const;

    std::unique_ptr<GeoJSONVT> m_store;
    std::vector<mapbox::util::geojsonvt::ProjectedFeature> m_features;

};

}
