#pragma once

#include "dataSource.h"
#include "util/types.h"

#include <mutex>
#include <unordered_map>

namespace mapbox {
namespace util {
namespace geojsonvt {
class GeoJSONVT;
class TilePoint;
class ProjectedFeature;
}}}

namespace Tangram {

using GeoJSONVT = mapbox::util::geojsonvt::GeoJSONVT;
using ProjectedFeature = mapbox::util::geojsonvt::ProjectedFeature;

struct Properties;

class ClientGeoJsonSource : public DataSource {

public:

    ClientGeoJsonSource(const std::string& _name, const std::string& _url,
                        int32_t _minDisplayZoom = -1, int32_t _maxDisplayZoom = -1, int32_t _maxZoom = 18);
    ~ClientGeoJsonSource();

    // Add geometry from a GeoJSON string
    void addData(const std::string& _data);
    void addPoint(const Properties& _tags, LngLat _point);
    void addLine(const Properties& _tags, const Coordinates& _line);
    void addPoly(const Properties& _tags, const std::vector<Coordinates>& _poly);
    void addFeature(ProjectedFeature& feature);
    void removeFeature(uint32_t featureId);

    virtual bool loadTileData(std::shared_ptr<TileTask>&& _task, TileTaskCb _cb) override;
    std::shared_ptr<TileTask> createTask(TileID _tileId, int _subTask) override;

    virtual void cancelLoadingTile(const TileID& _tile) override {};
    virtual void clearData() override;

protected:

    virtual std::shared_ptr<TileData> parse(const TileTask& _task,
                                            const MapProjection& _projection) const override;

    std::unique_ptr<GeoJSONVT> m_store;
    mutable std::mutex m_mutexStore;
    std::vector<ProjectedFeature> m_features;
    uint32_t m_maxFeatureId;
    std::unordered_map<uint32_t, size_t> m_featureIdMap;
    bool m_hasPendingData = false;

};

}
