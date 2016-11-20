#pragma once

#include "data/tileData.h"
#include "pbf/pbf.hpp"
#include "util/variant.h"

#include <vector>
#include <string>

namespace Tangram {

namespace PbfParser {

    using StringView = std::pair<size_t, const char*>;
    using TagValue = variant<StringView, double, none_type>;

    struct ParserContext {
        ParserContext(int32_t _sourceId) {
            values.reserve(256);
            feature.props.sourceId = _sourceId;
            feature.geometry.points().reserve(512);
        }

        std::vector<std::string> keys;
        std::vector<TagValue> values;
        std::vector<protobuf::message> featureMsgs;
        // Map Key ID -> Tag values
        std::vector<int> featureTags;
        std::vector<int> previousTags;
        // Key IDs sorted by Property key ordering
        std::vector<int> orderedKeys;
        Feature feature;

        int tileExtent = 0;
        int winding = 0;
    };

    void extractGeometry(ParserContext& _ctx, protobuf::message& _geomIn);

    bool extractTags(ParserContext& _ctx, protobuf::message& _tagsIn);

    void extractFeature(ParserContext& _ctx, protobuf::message& _featureIn, TileDataSink& _sink);

    void extractLayer(ParserContext& _ctx, protobuf::message& _in, TileDataSink& _sink);

    enum pbfGeomCmd {
        moveTo = 1,
        lineTo = 2,
        closePath = 7
    };
}

}
