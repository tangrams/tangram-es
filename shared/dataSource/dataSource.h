/*
...
*/

#ifndef __DATA_SOURCE_H__
#define __DATA_SOURCE_H__

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

//Todo: Make DataSource an abstract base class and
//Todo: Implement MapzenVectorTile which extends DataSource
class DataSource {
protected:
    // map of tile coordinates (as a string: x_y_level) to json data for that tile
    std::map< std::string, std::shared_ptr<Json::Value> > m_JsonRoots;
    //Json::Value m_JsonRoot;

public:
    virtual std::vector<glm::vec3> LoadTile() = 0;
    virtual std::shared_ptr<Json::Value> GetData(std::string tileID) = 0;
    std::vector<glm::vec3> LoadGeoJsonFile();
    void ClearGeoRoots();
    std::shared_ptr<Json::Value> GetGeoJson(std::string tileID);
    //Json::Value GetGeoJson();
    DataSource() {}
    ~DataSource() {
        for (auto& mapValue : m_JsonRoots) {
            mapValue.second->clear();
        }
        m_JsonRoots.clear();
    }
};

//Extends DataSource class to read MapzenVectorTileJsons.
class MapzenVectorTileJson: public DataSource {
public:
    MapzenVectorTileJson() {}
    virtual std::vector<glm::vec3> LoadTile();
    virtual std::shared_ptr<Json::Value> GetData(std::string tileID);
};

#endif