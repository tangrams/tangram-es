#pragma once

#include <vector>

#include "json/json.h"
#include "glm/glm.hpp"

#include "projection.h"

namespace JsonExtractor {

    //TODO: add methods to extract json data into tile data encapsulation
    
    /* method to extract geometry coordinates from json into a vector of vec3 */
    void extractGeomCoords(std::vector<glm::vec3>& _outGeomCoords, std::vector<int>& _ringSizes, const Json::Value& _featureJson, const glm::dvec2& _tileOrigin, const MapProjection& _mapProjection, int _multiPolySize = 1);

    /* method to extract geometry type of the feature */
    std::string extractGeomType(const Json::Value& _featureJson);

    /* method to extract the number of polygons in a multipoly geometry type feature */
    int extractNumPoly(const Json::Value& _featureJson);
}


