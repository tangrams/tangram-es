/*
...
*/
#pragma once

#include "json/json.h"

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <sstream>

#include "glm/glm.hpp"
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
    // map of tileIDs to json data for that tile
    std::map< TileID, std::shared_ptr<Json::Value> > m_JsonRoots;
    
public:
    DataSource() {}
    
    /*
     * Does all the curl network calls to load the tile data and fills the data associated with a tileID
     */
    virtual bool loadTile(const std::vector<TileID>& _tileCoords) = 0;
    
    /*
     * Returns the data corresponding to a tileID
     */
    virtual std::shared_ptr<Json::Value> getData(const TileID& _tileID);
    
    /*
     * Checks if data exists for a specific tileID 
     */
    virtual bool checkDataExists(const TileID& _tileID);
    
    /* 
     * clears all data associated with this dataSource
     */
    void clearGeoRoots();
    
    /*
     * returns the number of tiles having data wrt this datasource
     */
    size_t jsonRootSize();
    
    virtual ~DataSource() {
        m_JsonRoots.clear();
    }
};

class NetworkDataSource : public DataSource {
protected:
    /* 
     * m_urlTemplate needs to be defined for every network dataSource
     */
    std::string m_urlTemplate;
    
    /*
     * constructs the URL for a tile based on tile coordinates/IDs.
     * Used by LoadTile to construct URL
     */
    virtual std::unique_ptr<std::string> constructURL(const TileID& _tileID);
    
    /* 
     * extracts tileIDs from a url
     * Used by LoadTile to extract tileIDs from curl url.
     * NOTE: every tile source will implement its own extractID as order of x,y and z can be different
     *       example: mapzen vector tile has z/x/y in its url
     */
    virtual TileID extractIDFromUrl(const std::string& _url) = 0;

public:
    NetworkDataSource() {};
    virtual ~NetworkDataSource() {
        m_urlTemplate.clear();
    }
};

//Extends NetworkDataSource class to read MapzenVectorTileJsons.
class MapzenVectorTileJson : public NetworkDataSource {
private:
    virtual TileID extractIDFromUrl(const std::string& _url) override;
public:
    MapzenVectorTileJson();
    virtual bool loadTile(const std::vector<TileID>& _tileCoords) override;
    virtual ~MapzenVectorTileJson() {}
};

class TopoJsonNetSrc : public NetworkDataSource {
};

class MapboxFormatNetSrc : public NetworkDataSource {
};

// --Support for tiled geoJson but no network (basically supports a geojson on the filesystem)
class GeoJsonFileSrc : public DataSource {
};
