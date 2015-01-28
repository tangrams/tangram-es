#pragma once

#include <vector>

#include "json/json.h"

#include "mapTile.h"
#include "tileData.h"

namespace GeoJson {
    
    void extractPoint(const Json::Value& _in, Point& _out, const MapTile& _tile);
    
    void extractLine(const Json::Value& _in, Line& _out, const MapTile& _tile);
    
    void extractPoly(const Json::Value& _in, Polygon& _out, const MapTile& _tile);
    
    void extractFeature(const Json::Value& _in, Feature& _out, const MapTile& _tile);
    
    void extractLayer(const Json::Value& _in, Layer& _out, const MapTile& _tile);
    
}


