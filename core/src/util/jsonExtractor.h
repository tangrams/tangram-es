#pragma once

#include <vector>

#include "json/json.h"
#include "glm/glm.hpp"

#include "mapProjection.h"

namespace JsonExtractor {

    //TODO: add methods to extract json data into tile data encapsulation
    
    /* method to extract geometry coordinates from json into a vector of vec3 */
    void extractGeomCoords(std::vector<glm::vec3>& _outGeomCoords, std::vector<int>& _ringSizes, const Json::Value& _featureJson, const glm::dvec2& _tileOrigin, const MapProjection& _mapProjection, bool _multiGeom = false);

    /* method to extract geometry type of the feature */
    std::string extractGeomType(const Json::Value& _featureJson);

    /* method to extract the number of polygons in a multipoly geometry type feature */
    int extractNumPoly(const Json::Value& _featureJson);

    /* method to extract height properties of a feature */
    void extractFeatureHeightProps(const Json::Value& _featureJson, float& _featureHeight, float& _minFeatureHeight);
    
    void extractPoint(const Json::Value& _in, glm::vec3& _out, const MapProjection& _proj, const glm::dvec2& _offset);
    
    void extractLine(const Json::Value& _in, std::vector<glm::vec3>& _out, const MapProjection& _proj, const glm::dvec2& _offset);
    
    void extractPoly(const Json::Value& _in, std::vector<glm::vec3>& _out, std::vector<int>& _outSizes, const MapProjection& _proj, const glm::dvec2& _offset);
    
}


