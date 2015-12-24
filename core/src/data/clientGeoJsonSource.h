#pragma once

#include "dataSource.h"
#include "util/types.h"

#include <mutex>

namespace mapbox {
namespace util {
namespace geojsonvt {
class GeoJSONVT;
class TilePoint;
class ProjectedFeature;
}}}

namespace Tangram {

using GeoJSONVT = mapbox::util::geojsonvt::GeoJSONVT;

struct Properties;

class ClientGeoJsonSource : public DataSource {

public:

    ClientGeoJsonSource(const std::string& _name, const std::string& _url);
    ~ClientGeoJsonSource();

    // Add geometry from a GeoJSON string
    void addData(const std::string& _data);
    void addPoint(const Properties& _tags, LngLat _point);
    void addLine(const Properties& _tags, const Coordinates& _line);
    void addPoly(const Properties& _tags, const std::vector<Coordinates>& _poly);

    virtual bool loadTileData(std::shared_ptr<TileTask>&& _task, TileTaskCb _cb) override;
    virtual bool getTileData(std::shared_ptr<TileTask>& _task) override;
    virtual void cancelLoadingTile(const TileID& _tile) override {};
    virtual void clearData() override;

protected:

    virtual std::shared_ptr<TileData> parse(const TileID& _tileId, const MapProjection& _projection,
                                            std::vector<char>& _rawData) const override;

    std::unique_ptr<GeoJSONVT> m_store;
    mutable std::mutex m_mutexStore;
    std::vector<mapbox::util::geojsonvt::ProjectedFeature> m_features;

};

}
