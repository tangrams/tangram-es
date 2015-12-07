#pragma once

#include <string>
#include <memory>
#include <vector>

namespace Tangram {

struct TileData;
struct TileID;
class Tile;
class TileManager;
struct RawCache;
class TileTask;
struct TileTaskCb;

class DataSource {

public:

    /* Tile data sources must have a name and a URL template that defines where to find
     * a tile based on its coordinates. A URL template includes exactly one occurrance
     * each of '{x}', '{y}', and '{z}' which will be replaced by the x index, y index,
     * and zoom level of tiles to produce their URL.
     */
    DataSource(const std::string& _name, const std::string& _urlTemplate);

    virtual ~DataSource();

    /* Fetches data for the map tile specified by @_tileID
     *
     * LoadTile starts an asynchronous I/O task to retrieve the data for a tile. When
     * the I/O task is complete, the tile data is added to a queue in @_tileManager for
     * further processing before it is renderable.
     */
    virtual bool loadTileData(std::shared_ptr<TileTask>&& _task, TileTaskCb _cb);

    /* Lookup TileData in cache and */
    virtual bool getTileData(std::shared_ptr<TileTask>& _task);

    /* Stops any running I/O tasks pertaining to @_tile */
    virtual void cancelLoadingTile(const TileID& _tile);

    /* Parse an I/O response into a <TileData>, returning an empty TileData on failure */
    virtual std::shared_ptr<TileData> parse(const Tile& _tile, std::vector<char>& _rawData) const = 0;

    /* Clears all data associated with this DataSource */
    virtual void clearData();

    const std::string& name() const { return m_name; }

    virtual bool equals(const DataSource& _other) const {
        return m_name == _other.m_name &&
               m_urlTemplate == _other.m_urlTemplate;
    }

    /* @_cacheSize: Set size of in-memory cache for tile data in bytes.
     * This cache holds unprocessed tile data for fast recreation of recently used tiles.
     */
    void setCacheSize(size_t _cacheSize);

    /* ID of this DataSource instance */
    int32_t id() { return m_id; }

    /* Generation ID of DataSource state (incremented for each update, e.g. on clearData()) */
    int64_t generation() { return m_generation; }

protected:

    void onTileLoaded(std::vector<char>&& _rawData, std::shared_ptr<TileTask>& _task, TileTaskCb _cb);

    /* Constructs the URL of a tile using <m_urlTemplate> */
    virtual void constructURL(const TileID& _tileCoord, std::string& _url) const;

    std::string constructURL(const TileID& _tileCoord) const {
        std::string url;
        constructURL(_tileCoord, url);
        return url;
    }

    // Name used to identify this source in the style sheet
    std::string m_name;

    // Unique id for DataSource
    int32_t m_id;

    // Generation of dynamic DataSource state (incremented for each update)
    int64_t m_generation = 1;

    // URL template for requesting tiles from a network or filesystem
    std::string m_urlTemplate;

    std::unique_ptr<RawCache> m_cache;
};

}
