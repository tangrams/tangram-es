#pragma once

#include <string>
#include <map>
#include <memory>
#include <vector>
#include <mutex>
#include "tile/tileTask.h"

struct TileData;
struct TileID;
class MapTile;
class TileManager;

class DataSource {
    
public:

    /* Tile data sources must have a name and a URL template that defines where to find 
     * a tile based on its coordinates. A URL template includes exactly one occurrance 
     * each of '{x}', '{y}', and '{z}' which will be replaced by the x index, y index, 
     * and zoom level of tiles to produce their URL
     */
    DataSource(const std::string& _name, const std::string& _urlTemplate);

    virtual ~DataSource() {}
    
    /* Fetches data for the map tile specified by @_tileID
     *
     * LoadTile starts an asynchronous I/O task to retrieve the data for a tile. When
     * the I/O task is complete, the tile data is added to a queue in @_tileManager for 
     * further processing before it is renderable. 
     */
    virtual bool loadTileData(TileTask _task, TileTaskCb _cb);

    /* Stops any running I/O tasks pertaining to @_tile */
    virtual void cancelLoadingTile(const TileID& _tile);

    /* Checks if data exists for a specific <TileID> */
    virtual bool hasTileData(const TileID& _tileID) const;

    /* Returns the data corresponding to a <TileID>, if it has been fetched already */
    virtual std::shared_ptr<TileData> getTileData(const TileID& _tileID) const;
    
    /* Parse an I/O response into a <TileData>, returning an empty TileData on failure */
    virtual std::shared_ptr<TileData> parse(const MapTile& _tile, std::vector<char>& _rawData) const = 0;

    /* Stores tileData in m_tileStore */
    virtual void setTileData(const TileID& _tileID, const std::shared_ptr<TileData>& _tileData);
    
    /* Clears all data associated with this DataSource */
    void clearData();

protected:

    /* Constructs the URL of a tile using <m_urlTemplate> */
    virtual void constructURL(const TileID& _tileCoord, std::string& _url) const;

    std::string constructURL(const TileID& _tileCoord) const {
        std::string url;
        constructURL(_tileCoord, url);
        return url;
    }
    
    std::map< TileID, std::shared_ptr<TileData> > m_tileStore; // Map of tileIDs to data for that tile
    
    std::string m_name; // Name used to identify this source in the style sheet

    std::mutex m_mutex; // Used to ensure safe access from async loading threads

    std::string m_urlTemplate; // URL template for requesting tiles from a network or filesystem

};
