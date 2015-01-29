#pragma once

#include <vector>
#include <unordered_map>
#include <string>

#include "pbf/pbf.hpp"

#include "mapTile.h"
#include "tileData.h"

namespace PbfParser {
    
    void extractPoint(const protobuf::message& _in, Point& _out, const MapTile& _tile);
    
    void extractLine(const protobuf::message& _in, Line& _out, const MapTile& _tile);
    
    void extractPoly(const protobuf::message& _in, Polygon& _out, const MapTile& _tile);
    
    void extractFeature(const protobuf::message& _in, Feature& _out, const MapTile& _tile, std::vector<std::string>& _keys, std::unordered_map<int, float>& _numericValues, std::unordered_map<int, std::string>& _stringValues);
    
    void extractLayer(const protobuf::message& _in, Layer& _out, const MapTile& _tile);
    
}


//Note: should create a structure for this and have ++ operator overloading etc
// Not used right now, refer either google's parsing example or the pbf.hpp used in mapbox's native repo
namespace PBFHelper {
    
    void extractMessage(protobuf::message& _in, protobuf::message& _out);
    
}

