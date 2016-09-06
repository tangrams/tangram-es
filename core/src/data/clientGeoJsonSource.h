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

    ClientGeoJsonSource(const std::string& _name, const std::string& _url, int32_t _minDisplayZoom = 0, int32_t _maxZoom = 18);
    ~ClientGeoJsonSource();

    // Add geometry from a GeoJSON string
    void addData(const std::string& _data);
    void addPoint(const Properties& _tags, LngLat _point);
    void addLine(const Properties& _tags, const Coordinates& _line);
    void addPoly(const Properties& _tags, const std::vector<Coordinates>& _poly);

    virtual bool loadTileData(std::shared_ptr<TileTask>&& _task, TileTaskCb _cb) override;
    std::shared_ptr<TileTask> createTask(TileID _tileId, int _subTask) override;

    virtual void cancelLoadingTile(const TileID& _tile) override {};
    virtual void clearData() override;

protected:

    virtual std::shared_ptr<TileData> parse(const TileTask& _task,
                                            const MapProjection& _projection) const override;

    std::unique_ptr<GeoJSONVT> m_store;
    mutable std::mutex m_mutexStore;
    std::vector<mapbox::util::geojsonvt::ProjectedFeature> m_features;
    bool m_hasPendingData = false;

};

}
