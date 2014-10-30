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


class DataSource {

protected:
    
    /* Map of tileIDs to json data for that tile */
    std::map< TileID, std::shared_ptr<Json::Value> > m_JsonRoots;
    
public:
    
    /* Fetch data for a map tile
     *
     * LoadTile performs synchronous I/O to retrieve all needed data for a tile,
     * then stores it to be accessed via <GetData>. This method SHALL NOT be called
     * from the main thread. 
     */
    virtual bool loadTile(const TileID& _tileID) = 0;

    /* Returns the data corresponding to a <TileID> */
    virtual std::shared_ptr<Json::Value> getTileData(const TileID& _tileID);

    /* Checks if data exists for a specific <TileID> */
    virtual bool hasTileData(const TileID& _tileID);
    
    /* Clears all data associated with this dataSource */
    void clearData();

    DataSource() {}
    virtual ~DataSource() { m_JsonRoots.clear(); }
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
    virtual std::unique_ptr<std::string> constructURL(const TileID& _tileCoord);

public:

    NetworkDataSource();
    virtual ~NetworkDataSource();

    virtual bool loadTile(const TileID& _tileID) override;

};

/* Extends NetworkDataSource class to read Mapzen's GeoJSON vector tiles */
class MapzenVectorTileJson: public NetworkDataSource {

public:

    MapzenVectorTileJson();

};

// TODO: Support TopoJSON tiles
class TopoJsonNetSrc : public NetworkDataSource {
};

// TODO: Support Mapbox tiles
class MapboxFormatNetSrc : public NetworkDataSource {
};

// TODO: Support local GeoJSON tiles
class GeoJsonFileSrc : public DataSource {
};
