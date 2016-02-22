#include "util/pbfParser.h"

#include "data/propertyItem.h"
#include "data/tileSource.h"
#include "tile/tile.h"
#include "log.h"
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

    pbfGeomCmd cmd = pbfGeomCmd::moveTo;
    uint32_t cmdRepeat = 0;

    double invTileExtent = (1.0/(double)_ctx.tileExtent);

    int64_t x = 0;
    int64_t y = 0;

    size_t numCoordinates = 0;

    while(_geomIn.getData() < _geomIn.getEnd()) {

        if(cmdRepeat == 0) { // get new command, length and parameters..
            uint32_t cmdData = static_cast<uint32_t>(_geomIn.varint());
            cmd = static_cast<pbfGeomCmd>(cmdData & 0x7); //first 3 bits of the cmdData
            cmdRepeat = cmdData >> 3; //last 5 bits
        }

        if(cmd == pbfGeomCmd::moveTo || cmd == pbfGeomCmd::lineTo) { // get parameters/points
            // if cmd is move then move to a new line/set of points and save this line
            if(cmd == pbfGeomCmd::moveTo) {
                if(_ctx.coordinates.size() > 0) {
                    _ctx.numCoordinates.push_back(numCoordinates);
                }
                numCoordinates = 0;
            }

            x += _geomIn.svarint();
            y += _geomIn.svarint();

            // bring the points in 0 to 1 space
            Point p;
            p.x = invTileExtent * (double)x;
            p.y = invTileExtent * (double)(_ctx.tileExtent - y);

            if (numCoordinates == 0 || _ctx.coordinates.back() != p) {
                _ctx.coordinates.push_back(p);
                numCoordinates++;
            }
        } else if(cmd == pbfGeomCmd::closePath) {
            // end of a polygon, push first point in this line as last and push line to poly
            _ctx.coordinates.push_back(_ctx.coordinates[_ctx.coordinates.size() - numCoordinates]);
            _ctx.numCoordinates.push_back(numCoordinates + 1);
            numCoordinates = 0;
        }

        cmdRepeat--;
    }

    // Enter the last line
    if (numCoordinates > 0) {
        _ctx.numCoordinates.push_back(numCoordinates);
    }
}

struct string_visitor {
    using result_type = bool;
    Value& propValue;
    std::string& str;

    bool operator()(const PbfParser::StringView& v) const {
        str.assign(v.second, v.first);
        return true;
    }
    template<typename T>
    bool operator()(const T& v) const {
        propValue = v;
        return false;
    }
};
struct double_visitor {
    using result_type = bool;
    Value& propValue;

    bool operator()(const PbfParser::StringView& v) const {
        propValue = std::string(v.second, v.first);
        return false;
    }
    template<typename T>
    bool operator()(const T& v) const {
        propValue = v;
        return true;
    }
};
struct update_visitor {
    using result_type = bool;
    const PbfParser::TagValue& tagValue;
    Value& propValue;

    template<typename T>
    bool operator()(T& v) const {
        return PbfParser::TagValue::visit(tagValue, double_visitor{propValue});
    }
    bool operator()(std::string& v) const {
        return PbfParser::TagValue::visit(tagValue, string_visitor{propValue, v});
    }
};


void PbfParser::extractFeature(ParserContext& _ctx, protobuf::message& _featureIn, TileDataSink& _sink) {

    protobuf::message geometry;

    _ctx.coordinates.clear();
    _ctx.numCoordinates.clear();

    // keep previous tags to check which tags changed
    _ctx.featureTags.swap(_ctx.previousTags);

    // clear current tags
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

    // Only update keys and values that differ from last feature.
    bool matchingKeys = true;
    numTags = 0;

    for (int tagKey : _ctx.orderedKeys) {
        int tagValue = _ctx.featureTags[tagKey];

        if (tagValue >= 0) {
            auto& prop = properties[numTags++];

            if (matchingKeys && _ctx.previousTags[tagKey] < 0) {
                // Previous feature did not have this key
                matchingKeys = false;
            }
            if (matchingKeys && _ctx.previousTags[tagKey] == tagValue) {
                // Same tag as the previous feature
                continue;
            }
            if (!matchingKeys) {
                // Update key
                prop.key = _ctx.keys[tagKey];
            }

            // Update value, reuse string allocation
            Value::visit(prop.value, update_visitor{_ctx.values[tagValue], prop.value});

        } else if (matchingKeys && _ctx.previousTags[tagKey] >= 0) {
            // Properties key-set changed from last feature
            matchingKeys = false;
        }
    }

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
            int offset = 0;
            feature.lines.clear();
            for (int length : _ctx.numCoordinates) {
                if (length == 0) { continue; }

                Line line;
                line.reserve(length);

                line.insert(line.begin(),
                            _ctx.coordinates.begin() + offset,
                            _ctx.coordinates.begin() + offset + length);

                offset += length;
                feature.lines.emplace_back(std::move(line));
            }
            break;
        }
        case GeometryType::polygons: {
            int offset = 0;
            int winding = 0;

            feature.polygons.clear();

            for (int length : _ctx.numCoordinates) {
                if (length == 0) { continue; }
                auto it = _ctx.coordinates.begin() + offset;

                double area = signedArea(it, it + length);
                if (area == 0.0) {
                    offset += length;
                    continue;
                }
                int curWinding = area > 0 ? 1 : -1;

                // Polygons are in a flat list of rings. The first ring's winding
                // determines which winding is the beginning of a new polygon.
                if (winding == 0) {
                    winding = curWinding;
                }

                if (feature.polygons.empty() || winding == curWinding) {
                    // Start new polygon
                    feature.polygons.emplace_back();
                }

                Line line;
                line.reserve(length);

                if (winding > 0) {
                    line.insert(line.end(), it, it + length);
                } else {
                    // Reverse points
                    auto it = _ctx.coordinates.rend() - offset - length;
                    line.insert(line.end(), it, it + length);
                }
                offset += length;

                feature.polygons.back().push_back(std::move(line));
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

        switch(_layerIn.tag) {
            case LAYER_FEATURE: {
                if (!lastWasFeature) {
                    _ctx.featureMsgs.push_back(_layerIn);
                    lastWasFeature = true;
                }
                _layerIn.skip();
                continue;
            }
            case LAYER_KEY: {
                _ctx.keys.push_back(_layerIn.string());
                break;
            }
            case LAYER_VALUE: {
                protobuf::message valueItr = _layerIn.getMessage();

                while (valueItr.next()) {
                    switch (valueItr.tag) {
                        case 1: // string value
                            _ctx.values.push_back(valueItr.chunk());
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
        lastWasFeature = false;
    }

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

    _ctx.featureTags.assign(_ctx.keys.size(), -1);

    for (auto& featureItr : _ctx.featureMsgs) {
        do {
            auto featureMsg = featureItr.getMessage();

            extractFeature(_ctx, featureMsg, _sink);

        } while (featureItr.next() && featureItr.tag == LAYER_FEATURE);
    }
}

}
