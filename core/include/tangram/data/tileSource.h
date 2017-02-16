#pragma once

#include "tile/tileTask.h"

#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace Tangram {

class MapProjection;
struct TileData;
struct TileID;
struct Raster;
class Tile;
class TileManager;
struct RawCache;
class Texture;

class TileSource : public std::enable_shared_from_this<TileSource> {

public:

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

    /* Tile data sources must have a name and a URL template that defines where to find
     * a tile based on its coordinates. A URL template includes exactly one occurrance
     * each of '{x}', '{y}', and '{z}' which will be replaced by the x index, y index,
     * and zoom level of tiles to produce their URL.
     */
    TileSource(const std::string& _name, std::unique_ptr<DataSource> _sources,
               int32_t _minDisplayZoom = -1, int32_t _maxDisplayZoom = -1, int32_t _maxZoom = 18);

    virtual ~TileSource();

    /**
     * @return the mime-type of the DataSource.
     */
    virtual const char* mimeType() = 0;

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
    virtual std::shared_ptr<TileData> parse(const TileTask& _task, const MapProjection& _projection) const = 0;

    /* Clears all data associated with this TileSource */
    virtual void clearData();

    const std::string& name() const { return m_name; }

    virtual void clearRasters();
    virtual void clearRaster(const TileID& id);

    bool equals(const TileSource& _other) const;

    virtual std::shared_ptr<TileTask> createTask(TileID _tile, int _subTask = -1);

    /* ID of this TileSource instance */
    int32_t id() const { return m_id; }

    /* Generation ID of TileSource state (incremented for each update, e.g. on clearData()) */
    int64_t generation() const { return m_generation; }

    int32_t minDisplayZoom() const { return m_minDisplayZoom; }
    int32_t maxDisplayZoom() const { return m_maxDisplayZoom; }
    int32_t maxZoom() const { return m_maxZoom; }

    bool isActiveForZoom(const float _zoom) const {
        return _zoom >= m_minDisplayZoom && (m_maxDisplayZoom == -1 || _zoom <= m_maxDisplayZoom);
    }

    /* assign/get raster datasources to this datasource */
    void addRasterSource(std::shared_ptr<TileSource> _dataSource);
    auto& rasterSources() { return m_rasterSources; }
    const auto& rasterSources() const { return m_rasterSources; }

    bool generateGeometry() const { return m_generateGeometry; }
    void generateGeometry(bool generateGeometry) { m_generateGeometry = generateGeometry; }

    /* Avoid RTTI by adding a boolean check on the data source object */
    virtual bool isRaster() const { return false; }

protected:

    void createSubTasks(std::shared_ptr<TileTask> _task);

    // This datasource is used to generate actual tile geometry
    bool m_generateGeometry = false;

    // Name used to identify this source in the style sheet
    std::string m_name;

    // URL template for requesting tiles from a network or filesystem
    std::string m_urlTemplate;

    // The path to an mbtiles tile store. Empty string if not present.
    std::string m_mbtilesPath;

    // Minimum zoom for which tiles will be displayed
    int32_t m_minDisplayZoom;

    // Maximum zoom for which tiles will be displayed
    int32_t m_maxDisplayZoom;

    // Maximum zoom for which tiles will be requested
    int32_t m_maxZoom;

    // Unique id for TileSource
    int32_t m_id;

    // Generation of dynamic TileSource state (incremented for each update)
    int64_t m_generation = 1;

    /* vector of raster sources (as raster samplers) referenced by this datasource */
    std::vector<std::shared_ptr<TileSource>> m_rasterSources;

    std::unique_ptr<DataSource> m_sources;
};

}
