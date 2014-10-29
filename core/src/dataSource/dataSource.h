#pragma once

#include "json/json.h"

#include <string>
#include <vector>
#include <map>
#include <memory>

#include "util/tileID.h"
#include "platform.h"

//Todo: Impelement TileData, a generic datastore for all tile formats,
//Have an instance of this in DataSource
//Every implementation of a DataSource will fill this TileData instance.
//Example MapzenVectorTile will read the json and fill this TileData
class TileData {
};


// TODO: divide DataSource into network and non-network dataSources.
//       Same has been done on the webgl tangram. Follow the same pattern.
class DataSource {

protected:
    
    /* Map of tileIDs to json data for that tile */
    std::map< TileID, std::shared_ptr<Json::Value> > m_JsonRoots;

    /* URL template for network data sources 
     *
     * Network data sources must define a URL template including exactly one 
     * occurrance each of '[x]', '[y]', and '[z]' which will be replaced by
     * the x index, y index, and zoom level of tiles to produce their URL
     */
    std::string m_urlTemplate;

public:
    
    /* Fetch data for a map tile
     *
     * LoadTile performs synchronous I/O to retrieve all needed data for a tile,
     * then stores it to be accessed via <GetData>. This method SHALL NOT be called
     * from the main thread. 
     */
    virtual bool loadTile(const TileID& _tileID) = 0;

    /* Returns the data corresponding to a <TileID> */
    virtual std::shared_ptr<Json::Value> getTileData(const TileID& _tileID) = 0;

    /* Checks if data exists for a specific <TileID> */
    virtual bool hasTileData(const TileID& _tileID) = 0;

    /* Constructs the URL of a tile using the 
     */
    virtual std::unique_ptr<std::string> constructURL(const TileID& _tileCoord) = 0;
    
    /* Clears all data associated with this dataSource */
    void clearData();

    DataSource() {}
    virtual ~DataSource() { m_JsonRoots.clear(); }
};

/* Extends DataSource class to read Mapzen's GeoJSON vector tiles */
class MapzenVectorTileJson: public DataSource {

public:
    MapzenVectorTileJson();
    
    virtual bool loadTile(const TileID& _tileID) override;
    virtual std::shared_ptr<Json::Value> getTileData(const TileID& _tileID) override;
    virtual bool hasTileData(const TileID& _tileID) override;
    virtual std::unique_ptr<std::string> constructURL(const TileID& _tileCoord) override;
    virtual ~MapzenVectorTileJson();
};

