#pragma once

#include "data/tileData.h"
#include "pbf/pbf.hpp"

#include <vector>
#include <string>

namespace Tangram {

class Tile;

namespace PbfParser {
    
    void extractGeometry(protobuf::message& _geomIn, int _tileExtent, std::vector<Line>& _out);
    
    void extractFeature(protobuf::message& _featureIn, Feature& _out, std::vector<std::string>& _keys, std::vector<float>& _numericValues, std::vector<std::string>& _stringValues, int _tileExtent);
    
    void extractLayer(protobuf::message& _in, Layer& _out);
    
    enum pbfGeomCmd {
        moveTo = 1,
        lineTo = 2,
        closePath = 7
    };
}

}
