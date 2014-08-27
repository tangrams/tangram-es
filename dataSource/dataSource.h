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

//Todo: Make DataSource an abstract base class and
//Todo: Implement VectorTileDataSource which extends DataSource

class DataSource {
    // map of tile coordinates (as a string: x_y_level) to json data for that tile
    std::map< std::string, std::shared_ptr<Json::Value> > m_JsonRoots;
    //Json::Value m_JsonRoot;

public:
    std::vector<glm::vec3> LoadGeoJsonFile();
    void ClearGeoRoots();
    std::shared_ptr<Json::Value> GetGeoJson(std::string tileID);
    //Json::Value GetGeoJson();
    DataSource() {}
    ~DataSource() {
        m_JsonRoots.clear();
    }
};

class VectorTileDataSource: public DataSource {

};

#endif