#include "dataSource.h"
#include <fstream>
#include <iostream>
#include <memory>


//--- DataSource Implementation ----
std::vector<glm::vec3> DataSource::LoadGeoJsonFile() {

    // tildID and FileName is explicitly hard coded, these
    // will be read from the service eventually using libCurl

    std::vector<glm::vec3> tileIDs;
    tileIDs.push_back(glm::vec3(1, 2, 3));
    //Need to do this to have a std::map, which respects only
    std::string tileID("1_2_3");
    std::string FileName("/Users/Varun/Development/tangram-es/data/test.json");

    // Json Extracting
    // TODO: Try Janson Library or rapidJson library
    std::shared_ptr<Json::Value> jsonLocalValue(new Json::Value);
    std::fstream inputStream(FileName, std::fstream::in);
    inputStream.seekg (0, inputStream.end);
    int length = inputStream.tellg();
    inputStream.seekg (0, inputStream.beg);
    char *jsonFileBuffer = new char[length];
    inputStream.read(jsonFileBuffer, length);
    Json::Reader jsonReader;
    jsonReader.parse(jsonFileBuffer, jsonFileBuffer + length, *(jsonLocalValue.get()));
    m_JsonRoots[tileID] = jsonLocalValue;
    return std::move(tileIDs);
}

void DataSource::ClearGeoRoots() {
    for (auto& mapValue : m_JsonRoots) {
        mapValue.second->clear();
    }
    m_JsonRoots.clear();
}

std::shared_ptr<Json::Value> DataSource::GetGeoJson(std::string TileID) {
    return m_JsonRoots[TileID];
}



//--- MapzenVectorTileJson Implementation
std::vector<glm::vec3> MapzenVectorTileJson::LoadTile() {

    // tildID and FileName is explicitly hard coded, these
    // will be read from the service eventually using libCurl

    std::vector<glm::vec3> tileIDs;
    tileIDs.push_back(glm::vec3(1, 2, 3));
    //Need to do this to have a std::map, which respects only
    std::string tileID("1_2_3");
    std::string FileName("/Users/Varun/Development/tangram-es/data/test.json");

    // Json Extracting
    // TODO: Try Janson Library or rapidJson library
    std::shared_ptr<Json::Value> jsonLocalValue(new Json::Value);
    std::fstream inputStream(FileName, std::fstream::in);
    inputStream.seekg (0, inputStream.end);
    int length = inputStream.tellg();
    inputStream.seekg (0, inputStream.beg);
    char *jsonFileBuffer = new char[length];
    inputStream.read(jsonFileBuffer, length);
    Json::Reader jsonReader;
    jsonReader.parse(jsonFileBuffer, jsonFileBuffer + length, *(jsonLocalValue.get()));
    m_JsonRoots[tileID] = jsonLocalValue;
    return std::move(tileIDs);
}

std::shared_ptr<Json::Value> MapzenVectorTileJson::GetData(std::string TileID) {
    return m_JsonRoots[TileID];
}
