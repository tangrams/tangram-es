#pragma once

#include <vector>
#include <unordered_map>
#include <string>

#include "pbf/pbf.hpp"

#include "mapTile.h"
#include "tileData.h"

namespace PbfParser {
    
    void extractGeometry(const protobuf::message& _in, uint32_t _tileExtent, std::vector<Line>& _out, const MapTile& _tile);
    
    void extractFeature(const protobuf::message& _in, Feature& _out, const MapTile& _tile, std::vector<std::string>& _keys, std::unordered_map<int, float>& _numericValues, std::unordered_map<int, std::string>& _stringValues, uint32_t _tileExtent);
    
    void extractLayer(const protobuf::message& _in, Layer& _out, const MapTile& _tile);
    
    enum pbfGeomCmd {
        moveTo = 1,
        lineTo = 2,
        closePath = 7
    };
}
