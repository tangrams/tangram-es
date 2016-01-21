#include "pbfParser.h"

#include "data/propertyItem.h"
#include "data/dataSource.h"
#include "tile/tile.h"
#include "platform.h"
#include "util/geom.h"

#include <algorithm>

#define FEATURE_ID 1
#define FEATURE_TAGS 2
#define FEATURE_TYPE 3
#define FEATURE_GEOM 4


#define LAYER_FEATURE 2
#define LAYER_KEY 3
#define LAYER_VALUE 4
#define LAYER_TILE_EXTENT 5

namespace Tangram {

void PbfParser::extractGeometry(ParserContext& _ctx, protobuf::message& _geomIn) {

    int32_t x = 0;
    int32_t y = 0;
    int32_t lastX = 0;
    int32_t lastY = 0;

    auto& coordinates = _ctx.coordinates;
    auto& numCoordinates = _ctx.numCoordinates;
    size_t cnt = 0;

    while(_geomIn.getData() < _geomIn.getEnd()) {

        uint32_t cmdData = static_cast<uint32_t>(_geomIn.varint());
        pbfGeomCmd cmd = static_cast<pbfGeomCmd>(cmdData & 0x7); // first 3 bits of the cmdData
        uint32_t cmdRepeat = cmdData >> 3; // last 5 bits

        if (cmd == pbfGeomCmd::lineTo) {

            for (uint32_t i = 0; i < cmdRepeat; ++i) {
                x += _geomIn.svarint();
                y += _geomIn.svarint();

                if (lastX != x || lastY != y || cnt == 0) {
                    lastX = x;
                    lastY = y;
                    coordinates.emplace_back(x, y, 0);
                    cnt++;
                }
            }

        } else if (cmd == pbfGeomCmd::moveTo) {

            // if cmd is move then move to a new line/set of points and save this line
            if (coordinates.size() > 0) {
                numCoordinates.push_back(cnt);
            }
            cnt = 0;
            for (uint32_t i = 0; i < cmdRepeat; ++i) {
                x += _geomIn.svarint();
                y += _geomIn.svarint();
                coordinates.emplace_back(x, y, 0);
                cnt++;
            }

        } else if (cmd == pbfGeomCmd::closePath) {
            // end of a polygon, push first point in this line as last and push line to poly
            coordinates.push_back(_ctx.coordinates[coordinates.size() - cnt]);
            numCoordinates.push_back(cnt + 1);
            cnt = 0;
        }
    }

    assert(_geomIn.getData() == _geomIn.getEnd());

    // Enter the last line
    if (cnt > 0) {
        numCoordinates.push_back(cnt);
    }

    // bring the points in 0 to 1 space
    double tileExtent = _ctx.tileExtent;
    double invTileExtent = 1.0/tileExtent;

    for (auto& p : coordinates) {
        p.x = invTileExtent * p.x;
        p.y = invTileExtent * (tileExtent - p.y);
    }
}

// Reuse string allocations in Value. Variant is not yet smart enough
// probably due to traits not being so well supported everywhere..
struct copy_visitor {
    using result_type = bool;
    template <typename T>
    bool operator()(T& l, const T& r) const {
        l = r;
        return true;
    }
    template <typename L, typename R>
    bool operator()(L l, R r) const {
        return false;
    }
};

void PbfParser::extractFeature(ParserContext& _ctx, protobuf::message& _featureIn, TileDataSink& _sink) {

    //Iterate through this feature
    protobuf::message geometry; // By default data_ and end_ are nullptr

    _ctx.coordinates.clear();
    _ctx.numCoordinates.clear();

    _ctx.featureTags.clear();
    _ctx.featureTags.assign(_ctx.keys.size(), -1);

    Feature& feature = _ctx.feature;
    size_t numTags = 0;

    while(_featureIn.next()) {
        switch(_featureIn.tag) {
            case FEATURE_ID:
                // ignored for now, also not used in json parsing
                _featureIn.skip();
                break;

            case FEATURE_TAGS: {
                protobuf::message tagsMsg = _featureIn.getMessage();

                while(tagsMsg) {
                    auto tagKey = tagsMsg.varint();

                    if(_ctx.keys.size() <= tagKey) {
                        LOGE("accessing out of bound key");
                        return;
                    }

                    if(!tagsMsg) {
                        LOGE("uneven number of feature tag ids");
                        return;
                    }

                    auto valueKey = tagsMsg.varint();

                    if( _ctx.values.size() <= valueKey ) {
                        LOGE("accessing out of bound values");
                        return;
                    }

                    _ctx.featureTags[tagKey] = valueKey;
                    numTags++;
                }
                break;
            }
            case FEATURE_TYPE:
                feature.geometryType = (GeometryType)_featureIn.varint();
                break;

            case FEATURE_GEOM:
                geometry = _featureIn.getMessage();
                extractGeometry(_ctx, geometry);
                break;

            default:
                _featureIn.skip();
                break;
        }
    }
    if (feature.geometryType == GeometryType::unknown) {
        return;
    }

    auto& properties = feature.props.items();
    properties.resize(numTags);

    numTags = 0;
    bool matchPrevious = true;

    int matchedKey = 0;
    int matchedVal = 0;
    int reused = 0;

    // Less copies: Only update keys and values that have changed
    for (int tagKey : _ctx.orderedKeys) {
        int tagValue = _ctx.featureTags[tagKey];

        if (tagValue >= 0) {

            auto& it = properties[numTags++];

            if (matchPrevious && _ctx.previousTags[tagKey] < 0) {
                matchPrevious = false;
            }
            if (matchPrevious && _ctx.previousTags[tagKey] == tagValue) {
                // Same as before. yay!
                matchedKey++;
                matchedVal++;
                continue;
            }
            if (!matchPrevious) {
                it.key = _ctx.keys[tagKey];
            } else {
                matchedKey++;
            }
            if (!Value::binary_visit(it.value, _ctx.values[tagValue], copy_visitor{})) {
                // Throw away old value
                it.value = _ctx.values[tagValue];
            } else { reused++; }
        } else {
            if (matchPrevious && _ctx.previousTags[tagKey] >= 0) {
                matchPrevious = false;
            }
        }
    }

    // LOG("Matched %d %d - %d / %d", matchedKey, matchedVal, reused + matchedVal, numTags);

    _ctx.featureTags.swap(_ctx.previousTags);

    if (!_sink.matchFeature(feature)) {
        return;
    }

    switch(feature.geometryType) {
    case GeometryType::points: {
            feature.points.clear();
            feature.points.insert(feature.points.begin(),
                               _ctx.coordinates.begin(),
                               _ctx.coordinates.end());
            break;
    }
    case GeometryType::lines: {
        feature.lines.resize(1);
        feature.lines[0].clear();

        auto coordIt = _ctx.coordinates.begin();
        size_t numLines = 0;

        for (int length : _ctx.numCoordinates) {
            if (length == 0) { continue; }

            if (numLines++ > 0) {
                feature.lines.emplace_back();
            }

            Line& line = feature.lines.back();
            line.reserve(length);
            line.insert(line.begin(), coordIt, coordIt + length);

            coordIt += length;
        }
        break;
    }
    case GeometryType::polygons: {
        feature.polygons.resize(1);
        feature.polygons[0].resize(1);
        feature.polygons[0][0].clear();

        auto coordIt = _ctx.coordinates.begin();
        size_t numRings = 0;

        for (int length : _ctx.numCoordinates) {
            if (length == 0) { continue; }

            // Polygons are in a flat list of rings, with ccw rings indicating
            // the beginning of a new polygon
            if (numRings++ > 0) {
                double area = signedArea(coordIt, coordIt + length);
                if (area > 0) {
                    feature.polygons.emplace_back();
                } else if (area == 0){
                    coordIt += length;
                    continue;
                }
                // Start ring
                feature.polygons.back().emplace_back();
            }

            auto& ring = feature.polygons.back().back();
            ring.reserve(length);
            ring.insert(ring.begin(), coordIt, coordIt + length);

            coordIt += length;
        }
        break;
    }
    default:
        break;
    }

    _sink.addFeature(feature);
}

void PbfParser::extractLayer(ParserContext& _ctx, protobuf::message& _layerIn, TileDataSink& _sink) {

    _ctx.keys.clear();
    _ctx.values.clear();
    _ctx.featureMsgs.clear();

    bool lastWasFeature = false;
    protobuf::message featureItr;

    // Iterate layer to populate featureMsgs, keys and values
    while(_layerIn.next()) {

        if (_layerIn.tag != LAYER_FEATURE) {
            lastWasFeature = false;
        }

        switch(_layerIn.tag) {
            case LAYER_FEATURE: {
                if (!lastWasFeature) {
                    _ctx.featureMsgs.push_back(_layerIn);
                    lastWasFeature = true;
                }
                _layerIn.skip();
                break;
            }
            case LAYER_KEY: {
                _ctx.keys.push_back(_layerIn.string());
                break;
            }
            case LAYER_VALUE:
            {
                protobuf::message valueItr = _layerIn.getMessage();

                while (valueItr.next()) {
                    switch (valueItr.tag) {
                        case 1: // string value
                            _ctx.values.push_back(valueItr.string());
                            break;
                        case 2: // float value
                            _ctx.values.push_back(valueItr.float32());
                            break;
                        case 3: // double value
                            _ctx.values.push_back(valueItr.float64());
                            break;
                        case 4: // int value
                            _ctx.values.push_back(valueItr.int64());
                            break;
                        case 5: // uint value
                            _ctx.values.push_back(valueItr.varint());
                            break;
                        case 6: // sint value
                            _ctx.values.push_back(valueItr.int64());
                            break;
                        case 7: // bool value
                            _ctx.values.push_back(valueItr.boolean());
                            break;
                        default:
                            _ctx.values.push_back(none_type{});
                            valueItr.skip();
                            break;
                    }
                }
                break;
            }
            case LAYER_TILE_EXTENT:
                _ctx.tileExtent = static_cast<int>(_layerIn.int64());
                break;

            default: // skip
                _layerIn.skip();
                break;
        }
    }

    // LOG("FEATURE MESSAGES %d  KEYS:%d VALUES:%d",
    //     _ctx.featureMsgs.size(), _ctx.keys.size(), _ctx.values.size());

    if (_ctx.featureMsgs.empty()) { return; }

    //// Assign ordering to keys for faster sorting
    _ctx.orderedKeys.clear();
    _ctx.orderedKeys.reserve(_ctx.keys.size());
    // assign key ids
    for (int i = 0, n = _ctx.keys.size(); i < n; i++) {
        _ctx.orderedKeys.push_back(i);
    }
    // sort by Property key ordering
    std::sort(_ctx.orderedKeys.begin(), _ctx.orderedKeys.end(),
              [&](int a, int b) {
                  return Properties::keyComparator(_ctx.keys[a], _ctx.keys[b]);
              });

    _ctx.previousTags.assign(_ctx.keys.size(), -1);

    for (auto& featureItr : _ctx.featureMsgs) {
        do {
            auto featureMsg = featureItr.getMessage();

            extractFeature(_ctx, featureMsg, _sink);

        } while (featureItr.next() && featureItr.tag == LAYER_FEATURE);
    }
}

}
