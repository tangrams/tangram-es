#pragma once

#include <vector>

#include "json/json.h"
#include "glm/glm.hpp"

#include "projection.h"

namespace JsonExtractor {

    //TODO: add methods to extract json data into tile data encapsulation
    
    /* method to extract geometry coordinates from json into a vector of vec3 */
    void extractGeomCoords(std::vector<glm::vec3>& _outGeomCoords, std::vector<int>& _ringSizes, const Json::Value& _featureJson, const glm::dvec2& _tileOrigin, const MapProjection& _mapProjection);

    /* method to extract geometry type of the feature */
    std::string extractGeomType(const Json::Value& _featureJson);

}


