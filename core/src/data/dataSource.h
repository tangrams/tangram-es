#pragma once

#include <string>
#include <sstream>
#include <map>
#include <memory>
#include <mutex>

struct TileData;
struct TileID;
class MapTile;

class DataSource {

protected:
    
    /* Map of tileIDs to data for that tile */
    std::map< TileID, std::shared_ptr<TileData> > m_tileStore;
    
    std::mutex m_mutex; // Used to ensure safe access from async loading threads
    
public:
    
    /* Fetch data for a map tile
     *
     * LoadTile performs synchronous I/O to retrieve all needed data for a tile,
     * then stores it to be accessed via <GetTileData>. This method SHALL NOT be called
     * from the main thread. 
     */
    virtual bool loadTileData(const TileID& _tile, const int _dataSourceID) = 0;

    /* Returns the data corresponding to a <TileID> */
    virtual std::shared_ptr<TileData> getTileData(const TileID& _tileID);

    /* Checks if data exists for a specific <TileID> */
    virtual bool hasTileData(const TileID& _tileID);
    
    /* Parse an I/O response into a <TileData>, returning an empty TileData on failure */
    virtual std::shared_ptr<TileData> parse(const MapTile& _tile, std::stringstream& _in) = 0;

    /* Stores tileData in m_tileStore */
    virtual void setTileData(const TileID& _tileID, const std::shared_ptr<TileData>& _tileData);
    
    /* Clears all data associated with this dataSource */
    void clearData();

    DataSource() {}
    virtual ~DataSource() { m_tileStore.clear(); }
};

class NetworkDataSource : public DataSource {

protected:

    /* URL template for network data sources 
     *
     * Network data sources must define a URL template including exactly one 
     * occurrance each of '[x]', '[y]', and '[z]' which will be replaced by
     * the x index, y index, and zoom level of tiles to produce their URL
     */
    std::string m_urlTemplate;

    /* Constructs the URL of a tile using <m_urlTemplate> */
    virtual void constructURL(const TileID& _tileCoord, std::string& _url);

public:

    NetworkDataSource();
    virtual ~NetworkDataSource();

    virtual bool loadTileData(const TileID& _tileID, const int _dataSourceID) override;

};

// TODO: Support TopoJSON tiles
class TopoJsonNetSrc : public NetworkDataSource {
};

// TODO: Support local GeoJSON tiles
class GeoJsonFileSrc : public DataSource {
};

