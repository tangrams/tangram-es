#include "jsonExtractor.h"
#include "platform.h"

void JsonExtractor::extractGeomCoords(std::vector<glm::vec3>& _outGeomCoords, std::vector<int>& _ringSizes, const Json::Value& _featureJson, const glm::dvec2& _tileOrigin, const MapProjection& _mapProjection, bool _multiGeom) {

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
    
    if(!_multiGeom) {
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
    } else {
        int nGeom = geometry.size();
        for(int poly = 0; poly < nGeom; poly++) {
            for(int i = 0; i < coordinates[poly].size(); i++) {
                int ringSize = coordinates[poly][i].size();
                //Iterate through all rings in the set and fill the coordinates[poly]
                for(int j = 0; j < ringSize; j++) {
                    glm::dvec2 tmp = _mapProjection.LonLatToMeters(glm::dvec2(coordinates[poly][i][j][0].asFloat(), coordinates[poly][i][j][1].asFloat()));
                    glm::dvec2 meters = tmp - _tileOrigin;
                    _outGeomCoords.push_back(glm::vec3(meters.x, meters.y, featureHeight));
                }
                if(ringSize != 0) {
                    _ringSizes.push_back(ringSize);
                }
            }
        }
    }

    //iterate through all sets of rings
}


std::string JsonExtractor::extractGeomType(const Json::Value& _featureJson) {
    return _featureJson["geometry"]["type"].asString();
}

int JsonExtractor::extractNumPoly(const Json::Value& _featureJson) {
    return _featureJson["geometry"]["coordinates"].size();
}

void JsonExtractor::extractPoint(const Json::Value& _in, glm::vec3& _out, const MapProjection& _proj, const glm::dvec2& _offset) {
    
    glm::dvec2 tmp = _proj.LonLatToMeters(glm::dvec2(_in[0].asDouble(), _in[1].asDouble()));
    _out.x = tmp.x - _offset.x;
    _out.y = tmp.y - _offset.y;
    
}

void JsonExtractor::extractLine(const Json::Value& _in, std::vector<glm::vec3>& _out, const MapProjection& _proj, const glm::dvec2& _offset) {
    
    for (auto point : _in) {
        glm::vec3 p;
        extractPoint(point, p, _proj, _offset);
        _out.push_back(p);
    }
    
}

void JsonExtractor::extractPoly(const Json::Value& _in, std::vector<glm::vec3>& _out, std::vector<int>& _outSizes, const MapProjection& _proj, const glm::dvec2& _offset) {
    
    for (auto line : _in) {
        extractLine(line, _out, _proj, _offset);
        _outSizes.push_back(line.size());
    }
    
}
