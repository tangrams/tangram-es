#pragma once

#include "data/tileSource.h"
#include "util/types.h"

#include <mutex>

namespace Tangram {

class Platform;

struct Properties;

class ClientDataSource : public TileSource {

public:

    ClientDataSource(Platform& _platform, const std::string& _name,
                        const std::string& _url, bool generateCentroids = false,
                        TileSource::ZoomOptions _zoomOptions = {});

    ~ClientDataSource() override;

    struct PolylineBuilderData;

    struct PolylineBuilder {
        PolylineBuilder();
        ~PolylineBuilder();
        void beginPolyline(size_t numberOfPoints);
        void addPoint(LngLat point);
        std::unique_ptr<PolylineBuilderData> data;
    };

    struct PolygonBuilderData;

    struct PolygonBuilder {
        PolygonBuilder();
        ~PolygonBuilder();
        void beginPolygon(size_t numberOfRings);
        void beginRing(size_t numberOfPoints);
        void addPoint(LngLat point);
        std::unique_ptr<PolygonBuilderData> data;
    };

    // http://www.iana.org/assignments/media-types/application/geo+json
    const char* mimeType() const override { return "application/geo+json"; };

    // Add geometry from a GeoJSON string
    void addData(const std::string& _data);

    void addPointFeature(Properties&& properties, LngLat coordinates);

    void addPolylineFeature(Properties&& properties, PolylineBuilder&& polyline);

    void addPolygonFeature(Properties&& properties, PolygonBuilder
        && polygon);

    // Transform added feature data into tiles.
    void generateTiles();

    void loadTileData(std::shared_ptr<TileTask> _task, TileTaskCb _cb) override;
    std::shared_ptr<TileTask> createTask(TileID _tileId) override;

    void cancelLoadingTile(TileTask& _task) override {};
    void clearData() override;

protected:

    std::shared_ptr<TileData> parse(const TileTask& _task) const override;

    struct Storage;
    std::unique_ptr<Storage> m_store;

    mutable std::mutex m_mutexStore;
    bool m_hasPendingData = false;
    bool m_generateCentroids = false;

    Platform& m_platform;

};

}
