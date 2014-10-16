#include "jsonExtractor.h"
#include "platform.h"

void JsonExtractor::extractGeomCoords(std::vector<glm::vec3>& _outGeomCoords, std::vector<int>& _ringSizes, const Json::Value& _featureJson, const glm::dvec2& _tileOrigin, const MapProjection& _mapProjection, int _multiPolySize) {

    float featureHeight = 0.0;
    float minFeatureHeight = 0.0;
    Json::Value geometry = _featureJson["geometry"];
    Json::Value property = _featureJson["properties"];
    Json::Value coordinates = geometry["coordinates"];

    extractFeatureHeightProps(_featureJson, featureHeight, minFeatureHeight);
    
    if(_multiPolySize == 1) {
        for(int i = 0; i < coordinates.size(); i++) {
            int ringSize = coordinates[i].size();
            //Iterate through all rings in the set and fill the coordinates
            for(int j = 0; j < ringSize; j++) {
                glm::dvec2 tmp = _mapProjection.LonLatToMeters(glm::dvec2(coordinates[i][j][0].asFloat(), coordinates[i][j][1].asFloat()));
                glm::dvec2 meters = tmp - _tileOrigin;
                if(minFeatureHeight != featureHeight) {
                    _outGeomCoords.push_back(glm::vec3(meters.x, meters.y, featureHeight));
                }
                else {
                    _outGeomCoords.push_back(glm::vec3(meters.x, meters.y, minFeatureHeight));
                }
            }
            if(ringSize != 0) {
                _ringSizes.push_back(ringSize);
            }
        }
    }

    else if(_multiPolySize > 1) {
        for(int poly = 0; poly < _multiPolySize; poly++) {
            for(int i = 0; i < coordinates[poly].size(); i++) {
                int ringSize = coordinates[poly][i].size();
                //Iterate through all rings in the set and fill the coordinates[poly]
                for(int j = 0; j < ringSize; j++) {
                    glm::dvec2 tmp = _mapProjection.LonLatToMeters(glm::dvec2(coordinates[poly][i][j][0].asFloat(), coordinates[poly][i][j][1].asFloat()));
                    glm::dvec2 meters = tmp - _tileOrigin;
                    if(minFeatureHeight != featureHeight) {
                        _outGeomCoords.push_back(glm::vec3(meters.x, meters.y, featureHeight));
                    }
                    else {
                        _outGeomCoords.push_back(glm::vec3(meters.x, meters.y, minFeatureHeight));
                    }
                }
                if(ringSize != 0) {
                    _ringSizes.push_back(ringSize);
                }
            }
        }
    }
    else {
        logMsg("\n***Negative value for multiPolySize (%d), not allowed. Check json data.", _multiPolySize);
    }
    //iterate through all sets of rings
}

void JsonExtractor::extractFeatureHeightProps(const Json::Value& _featureJson, float& _featureHeight, float& _minFeatureHeight) {
    Json::Value property = _featureJson["properties"];
    if(property.isMember("height")) {
        _featureHeight = property["height"].asFloat();
    }
    if(property.isMember("min_height")) {
        _minFeatureHeight = property["min_height"].asFloat();
    }
}


std::string JsonExtractor::extractGeomType(const Json::Value& _featureJson) {
    return _featureJson["geometry"]["type"].asString();
}

int JsonExtractor::extractNumPoly(const Json::Value& _featureJson) {
    return _featureJson["geometry"]["coordinates"].size();
}
