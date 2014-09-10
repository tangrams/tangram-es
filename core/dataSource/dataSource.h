/*
...
*/
#pragma once

#include "json/json.h"

#include <string>
#include <vector>
#include <map>
#include <memory>

#include "glm/glm.hpp"

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
    virtual bool LoadTile(std::vector<glm::ivec3> _tileCoords) = 0;
    virtual std::shared_ptr<Json::Value>
                	GetData(std::string _tileID) = 0;
    void ClearGeoRoots();
    DataSource() {}
    ~DataSource() {
        m_JsonRoots.clear();
    }
};

//Extends DataSource class to read MapzenVectorTileJsons.
class MapzenVectorTileJson: public DataSource {
public:
    MapzenVectorTileJson() {}
    virtual bool LoadTile(std::vector<glm::ivec3> _tileCoords) override;
    virtual std::shared_ptr<Json::Value>
                GetData(std::string _tileID) override;
};
