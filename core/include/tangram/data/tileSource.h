#pragma once

#include "tile/tileTask.h"

#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace Tangram {

struct TileData;
struct TileID;
struct Raster;
class Tile;
class TileManager;
struct RawCache;
class Texture;

class TileSource : public std::enable_shared_from_this<TileSource> {

public:

    struct ZoomOptions {

        ZoomOptions(int32_t _minDisplayZoom, int32_t _maxDisplayZoom,
                      int32_t _maxZoom, int32_t _zoomBias)
            : minDisplayZoom(_minDisplayZoom), maxDisplayZoom(_maxDisplayZoom),
              maxZoom(_maxZoom), zoomBias(_zoomBias) {}

        ZoomOptions() {}

        // Minimum zoom for which tiles will be displayed
        int32_t minDisplayZoom = -1;
        // Maximum zoom for which tiles will be displayed
        int32_t maxDisplayZoom = -1;
        // Maximum zoom for which tiles will be requested
        int32_t maxZoom = 18;
        // controls the zoom level for the tiles of the tilesource to scale the tiles to
        // apt pixel
        // 0: 256 pixel tiles
        // 1: 512 pixel tiles
        int32_t zoomBias = 0;
    };

    /* Calculate the zoom level bias to be applied given tileSize in pixel units.
     * 256  pixel -> 0
     * 512  pixel -> 1
     * 1024 pixel -> 2
     */
    static int32_t zoomBiasFromTileSize(int32_t tileSize);

    struct DataSource {
        virtual ~DataSource() {}

        virtual bool loadTileData(std::shared_ptr<TileTask> _task, TileTaskCb _cb) = 0;

        /* Stops any running I/O tasks pertaining to @_tile */
        virtual void cancelLoadingTile(const TileID& _tile) {
            if (next) { next->cancelLoadingTile(_tile); }
        }

        virtual void clear() { if (next) next->clear(); }

        void setNext(std::unique_ptr<DataSource> _next) {
            next = std::move(_next);
            next->level = level + 1;
        }
        std::unique_ptr<DataSource> next;
        int level = 0;
    };

    enum class Format {
        GeoJson,
        TopoJson,
        Mvt,
    };

    /* Tile data sources must have a name and a URL template that defines where to find
     * a tile based on its coordinates. A URL template includes exactly one occurrance
     * each of '{x}', '{y}', and '{z}' which will be replaced by the x index, y index,
     * and zoom level of tiles to produce their URL.
     */
    TileSource(const std::string& _name, std::unique_ptr<DataSource> _sources,
               ZoomOptions _zoomOptions = {});

    virtual ~TileSource();

    /**
     * @return the mime-type of the DataSource.
     */
    virtual const char* mimeType() const;

    /* Fetches data for the map tile specified by @_tileID
     *
     * LoadTile starts an asynchronous I/O task to retrieve the data for a tile. When
     * the I/O task is complete, the tile data is added to a queue in @_tileManager for
     * further processing before it is renderable.
     */
    virtual void loadTileData(std::shared_ptr<TileTask> _task, TileTaskCb _cb);

    /* Stops any running I/O tasks pertaining to @_tile */
    virtual void cancelLoadingTile(const TileID& _tile);

    /* Parse a <TileTask> with data into a <TileData>, returning an empty TileData on failure */
    virtual std::shared_ptr<TileData> parse(const TileTask& _task) const;

    /* Clears all data associated with this TileSource */
    virtual void clearData();

    const std::string& name() const { return m_name; }

    virtual void clearRasters();
    virtual void clearRaster(const TileID& id);

    virtual std::shared_ptr<TileTask> createTask(TileID _tile, int _subTask = -1);

    /* ID of this TileSource instance */
    int32_t id() const { return m_id; }

    /* Generation ID of TileSource state (incremented for each update, e.g. on clearData()) */
    int64_t generation() const { return m_generation; }

    const ZoomOptions& zoomOptions() { return m_zoomOptions; }
    int32_t minDisplayZoom() const { return m_zoomOptions.minDisplayZoom; }
    int32_t maxDisplayZoom() const { return m_zoomOptions.maxDisplayZoom; }
    int32_t maxZoom() const { return m_zoomOptions.maxZoom; }
    int32_t zoomBias() const { return m_zoomOptions.zoomBias; }

    bool isActiveForZoom(const float _zoom) const {
        return _zoom >= m_zoomOptions.minDisplayZoom &&
            (m_zoomOptions.maxDisplayZoom == -1 || _zoom <= m_zoomOptions.maxDisplayZoom);
    }

    /* assign/get raster datasources to this datasource */
    void addRasterSource(std::shared_ptr<TileSource> _dataSource);
    auto& rasterSources() { return m_rasterSources; }
    const auto& rasterSources() const { return m_rasterSources; }

    bool generateGeometry() const { return m_generateGeometry; }
    void generateGeometry(bool generateGeometry) { m_generateGeometry = generateGeometry; }

    /* Avoid RTTI by adding a boolean check on the data source object */
    virtual bool isRaster() const { return false; }

    void setFormat(Format format) { m_format = format; }

protected:

    void createSubTasks(std::shared_ptr<TileTask> _task);

    // This datasource is used to generate actual tile geometry
    bool m_generateGeometry = false;

    // Name used to identify this source in the style sheet
    std::string m_name;

    // zoom dependent props
    ZoomOptions m_zoomOptions;

    // Unique id for TileSource
    int32_t m_id;

    // Generation of dynamic TileSource state (incremented for each update)
    int64_t m_generation = 1;

    Format m_format = Format::GeoJson;

    /* vector of raster sources (as raster samplers) referenced by this datasource */
    std::vector<std::shared_ptr<TileSource>> m_rasterSources;

    std::unique_ptr<DataSource> m_sources;
};

}
