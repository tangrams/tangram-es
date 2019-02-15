#pragma once

#include "data/tileSource.h"
#include "util/types.h"

#include <mutex>


namespace Tangram {

class Platform;

struct Properties;

struct ClientGeoJsonData;

class ClientGeoJsonSource : public TileSource {

public:

    ClientGeoJsonSource(Platform& _platform, const std::string& _name,
                        const std::string& _url, bool generateCentroids = false,
                        TileSource::ZoomOptions _zoomOptions = {});

    ~ClientGeoJsonSource() override;

    // http://www.iana.org/assignments/media-types/application/geo+json
    const char* mimeType() const override { return "application/geo+json"; };

    // Add geometry from a GeoJSON string
    void addData(const std::string& _data);
    void addPoint(const Properties& _tags, LngLat _point);
    void addLine(const Properties& _tags, const Coordinates& _line);
    void addPoly(const Properties& _tags, const std::vector<Coordinates>& _poly);

    // Transform added feature data into tiles.
    void generateTiles();

    void loadTileData(std::shared_ptr<TileTask> _task, TileTaskCb _cb) override;
    std::shared_ptr<TileTask> createTask(TileID _tileId) override;

    void cancelLoadingTile(TileTask& _task) override {};
    void clearData() override;

protected:

    std::shared_ptr<TileData> parse(const TileTask& _task) const override;

    std::unique_ptr<ClientGeoJsonData> m_store;

    mutable std::mutex m_mutexStore;
    bool m_hasPendingData = false;
    bool m_generateCentroids = false;

    Platform& m_platform;

};

}
