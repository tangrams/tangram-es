#pragma once


#include <string>
#include <memory>
#include <vector>

#include "tile/tileTask.h"

namespace Tangram {

class MapProjection;
struct TileData;
struct TileID;
struct Raster;
class Tile;
class TileManager;
struct RawCache;
class Texture;

class DataSource : public std::enable_shared_from_this<DataSource> {

public:

    /* Tile data sources must have a name and a URL template that defines where to find
     * a tile based on its coordinates. A URL template includes exactly one occurrance
     * each of '{x}', '{y}', and '{z}' which will be replaced by the x index, y index,
     * and zoom level of tiles to produce their URL.
     */
    DataSource(const std::string& _name, const std::string& _urlTemplate,
               int32_t _minDisplayZoom = 0, int32_t _maxDisplayZoom = INT32_MAX, int32_t _maxZoom = 18);

    virtual ~DataSource();

    /* Fetches data for the map tile specified by @_tileID
     *
     * LoadTile starts an asynchronous I/O task to retrieve the data for a tile. When
     * the I/O task is complete, the tile data is added to a queue in @_tileManager for
     * further processing before it is renderable.
     */
    virtual bool loadTileData(std::shared_ptr<TileTask>&& _task, TileTaskCb _cb);


    /* Stops any running I/O tasks pertaining to @_tile */
    virtual void cancelLoadingTile(const TileID& _tile);

    /* Parse a <TileTask> with data into a <TileData>, returning an empty TileData on failure */
    virtual std::shared_ptr<TileData> parse(const TileTask& _task, const MapProjection& _projection) const = 0;

    /* Clears all data associated with this DataSource */
    virtual void clearData();

    const std::string& name() const { return m_name; }

    virtual void clearRasters();
    virtual void clearRaster(const TileID& id);

    bool equals(const DataSource& _other) const;

    virtual std::shared_ptr<TileTask> createTask(TileID _tile, int _subTask = -1);

    /* @_cacheSize: Set size of in-memory cache for tile data in bytes.
     * This cache holds unprocessed tile data for fast recreation of recently used tiles.
     */
    void setCacheSize(size_t _cacheSize);

    /* ID of this DataSource instance */
    int32_t id() const { return m_id; }

    /* Generation ID of DataSource state (incremented for each update, e.g. on clearData()) */
    int64_t generation() const { return m_generation; }

    int32_t minZoom() const { return m_minDisplayZoom; }
    int32_t maxZoom() const { return m_maxZoom; }

    bool isActiveForZoom(const float _zoom) const {
        return _zoom >= m_minDisplayZoom && _zoom <= m_maxDisplayZoom;
    }

    /* assign/get raster datasources to this datasource */
    auto& rasterSources() { return m_rasterSources; }
    const auto& rasterSources() const { return m_rasterSources; }

    bool generateGeometry() const { return m_generateGeometry; }
    void generateGeometry(bool generateGeometry) { m_generateGeometry = generateGeometry; }

    /* Avoid RTTI by adding a boolean check on the data source object */
    virtual bool isRaster() const { return false; }

protected:

    virtual void onTileLoaded(std::vector<char>&& _rawData, std::shared_ptr<TileTask>&& _task,
                              TileTaskCb _cb);

    /* Constructs the URL of a tile using <m_urlTemplate> */
    virtual void constructURL(const TileID& _tileCoord, std::string& _url) const;

    std::string constructURL(const TileID& _tileCoord) const {
        std::string url;
        constructURL(_tileCoord, url);
        return url;
    }

    bool cacheGet(DownloadTileTask& _task);

    void cachePut(const TileID& _tileID, std::shared_ptr<std::vector<char>> _rawDataRef);

    // This datasource is used to generate actual tile geometry
    bool m_generateGeometry = false;

    // Name used to identify this source in the style sheet
    std::string m_name;

    // Minimum zoom for which tiles will be displayed
    int32_t m_minDisplayZoom;

    // Maximum zoom for which tiles will be displayed
    int32_t m_maxDisplayZoom;

    // Maximum zoom for which tiles will be requested
    int32_t m_maxZoom;

    // Unique id for DataSource
    int32_t m_id;

    // Generation of dynamic DataSource state (incremented for each update)
    int64_t m_generation = 1;

    // URL template for requesting tiles from a network or filesystem
    std::string m_urlTemplate;

    std::unique_ptr<RawCache> m_cache;

    /* vector of raster sources (as raster samplers) referenced by this datasource */
    std::vector<std::shared_ptr<DataSource>> m_rasterSources;
};

}
