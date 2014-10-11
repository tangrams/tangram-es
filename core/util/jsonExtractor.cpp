#include "jsonExtractor.h"

void JsonExtractor::extractGeomCoords(std::vector<glm::vec3>& _outGeomCoords, std::vector<int>& _ringSizes, const Json::Value& _featureJson, const glm::dvec2& _tileOrigin, const MapProjection& _mapProjection) {

    float featureHeight = 0.0;
    float minFeatureHeight = 0.0;
    Json::Value geometry = _featureJson["geometry"];
    Json::Value property = _featureJson["properties"];
    Json::Value coordinates = geometry["coordinates"];
    
    if(property.isMember("height")) {
        featureHeight = property["height"].asFloat();
    }
    if(property.isMember("min_height")) {
        minFeatureHeight = property["min_height"].asFloat();
    }
    
    //iterate through all sets of rings
    for(int i = 0; i < coordinates.size(); i++) {
        int ringSize = coordinates[i].size();
        //Iterate through all rings in the set and fill the coordinates
        for(int j = 0; j < ringSize; j++) {
            glm::dvec2 tmp = _mapProjection.LonLatToMeters(glm::dvec2(coordinates[i][j][0].asFloat(), coordinates[i][j][1].asFloat()));
            glm::dvec2 meters = tmp - _tileOrigin;
            _outGeomCoords.push_back(glm::vec3(meters.x, meters.y, featureHeight));
        }
        if(ringSize != 0) {
            _ringSizes.push_back(ringSize);
        }
    }
}


std::string JsonExtractor::extractGeomType(const Json::Value& _featureJson) {
    return _featureJson["geometry"]["type"].asString();
}
