#pragma once

#include "data/tileData.h"
#include "pbf/pbf.hpp"
#include "util/variant.h"

#include <vector>
#include <string>

namespace Tangram {

class Tile;
struct TileDataSink;

namespace PbfParser {

    struct ParserContext {
        ParserContext(int32_t _sourceId) : sourceId(_sourceId){}

        int32_t sourceId;
        std::vector<std::string> keys;
        std::vector<Value> values;
        std::vector<protobuf::message> featureMsgs;
        std::vector<Point> coordinates;
        std::vector<int> numCoordinates;
        // Map Key ID -> Tag values
        std::vector<int> featureTags;
        std::vector<int> previousTags;
        // Key IDs sorted by Property key ordering
        std::vector<int> orderedKeys;
        Feature feature;

        int tileExtent = 0;
    };

    void extractGeometry(ParserContext& _ctx, protobuf::message& _geomIn);

    void extractFeature(ParserContext& _ctx, protobuf::message& _featureIn, TileDataSink& _sink);

    void extractLayer(ParserContext& _ctx, protobuf::message& _in, TileDataSink& _sink);

    enum pbfGeomCmd {
        moveTo = 1,
        lineTo = 2,
        closePath = 7
    };
}

}
