#pragma once

#include "data/tileData.h"
#include "pbf/pbf.hpp"
#include "util/variant.h"

#include <string>
#include <vector>

namespace Tangram {

class Tile;

namespace PbfParser {

    struct Geometry {
        std::vector<Point> coordinates;
        std::vector<int> sizes;
    };

    struct ParserContext {
        ParserContext(int32_t _sourceId) : sourceId(_sourceId){}

        int32_t sourceId;
        std::vector<std::string> keys;
        std::vector<Value> values;
        std::vector<protobuf::message> featureMsgs;
        Geometry geometry;
        // Map Key ID -> Tag values
        std::vector<int> featureTags;
        // Key IDs sorted by Property key ordering
        std::vector<int> orderedKeys;

        int tileExtent = 0;
        int winding = 0;
    };

    Geometry getGeometry(ParserContext& _ctx, protobuf::message _geomIn);

    Feature getFeature(ParserContext& _ctx, protobuf::message _featureIn);

    Layer getLayer(ParserContext& _ctx, protobuf::message _layerIn);

    enum pbfGeomCmd {
        moveTo = 1,
        lineTo = 2,
        closePath = 7
    };
}

}
