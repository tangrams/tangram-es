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
    // map of tile coordinates (as a string: x_y_level) to json data for that tile
    std::map< std::string, std::shared_ptr<Json::Value> > m_JsonRoots;

public:
    virtual bool LoadTile(std::vector<TileID> _tileCoords) = 0;
    virtual std::shared_ptr<Json::Value> GetData(std::string _tileID) = 0;
    virtual std::shared_ptr<Json::Value> GetData(TileID _tileID) = 0;
    virtual bool CheckDataExists(std::string _tileID) = 0;
    virtual bool CheckDataExists(TileID _tileID) = 0;
    void ClearGeoRoots();
    size_t JsonRootSize();
    DataSource() {}
    virtual ~DataSource() {
        m_JsonRoots.clear();
    }
};

//Extends DataSource class to read MapzenVectorTileJsons.
class MapzenVectorTileJson: public DataSource {
public:
    MapzenVectorTileJson() {}
    virtual bool LoadTile(std::vector<TileID> _tileCoords) override;
    virtual std::shared_ptr<Json::Value> GetData(std::string _tileID) override;
    virtual std::shared_ptr<Json::Value> GetData(TileID _tileID) override;
    virtual bool CheckDataExists(std::string _tileID) override;
    virtual bool CheckDataExists(TileID _tileID) override;
    virtual ~MapzenVectorTileJson() {}
};

//---- tileID and url construction----

//constructs a string from the tile coodinates

//constructs a mapzen vectortile json url from the tile coordinates
//TODO: Use regex to do this better.
static std::unique_ptr<std::string> constructURL(TileID _tileCoord) {
    std::ostringstream strStream;
    strStream<<"http://vector.mapzen.com/osm/all/"<<_tileCoord.z
                <<"/"<<_tileCoord.x<<"/"<<_tileCoord.y<<".json";
    std::unique_ptr<std::string> url(new std::string(strStream.str()));
    return std::move(url);
}

//TODO: Use regex to do this better.
// Hacking to extract id from url
static std::string extractIDFromUrl(std::string _url) {
    std::string baseURL("http://vector.mapzen.com/osm/all/");
    std::string jsonStr(".json");
    std::string tmpID = _url.replace(0, baseURL.length(), "");
    std::size_t jsonPos = tmpID.find(jsonStr);
    tmpID = tmpID.replace(jsonPos, jsonStr.length(), "");
    std::replace(tmpID.begin(), tmpID.end(), '/','_');
    return tmpID;
}
