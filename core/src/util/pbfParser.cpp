#include "pbfParser.h"

#include "data/propertyItem.h"
#include "data/dataSource.h"
#include "tile/tile.h"
#include "platform.h"
#include "util/geom.h"
#include "log.h"

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

    auto& geometry = _ctx.feature.geometry;
    auto& points = geometry.points();
    geometry.clear();

    size_t nPoints = 0;
    size_t nRings = 0;
    int winding = 0;

    double tileExtent = _ctx.tileExtent;
    double invTileExtent = 1.0/tileExtent;

    while(_geomIn.getData() < _geomIn.getEnd()) {

        uint32_t cmdData = static_cast<uint32_t>(_geomIn.varint());
        pbfGeomCmd cmd = static_cast<pbfGeomCmd>(cmdData & 0x7); // first 3 bits of the cmdData
        uint32_t cmdRepeat = cmdData >> 3; // last 5 bits

        if (cmd == pbfGeomCmd::lineTo ||
            cmd == pbfGeomCmd::moveTo) {

            // If cmd is move then save current line and start next line
            if (cmd == pbfGeomCmd::moveTo && nPoints > 0) {
                geometry.endLine();
                nPoints = 0;
            }

            for (uint32_t i = 0; i < cmdRepeat; ++i) {
                x += _geomIn.svarint();
                y += _geomIn.svarint();

                if (lastX != x || lastY != y || nPoints == 0) {
                    // Bring the points in 0 to 1 space
                    points.emplace_back(invTileExtent * x,
                                        invTileExtent * (tileExtent - y),
                                        0);
                    nPoints++;
                }
                lastX = x;
                lastY = y;
            }

        } else if (cmd == pbfGeomCmd::closePath) {
            // End of a polygon
            if (nPoints == 0) {
                assert(false);
                continue;
            }
            // Push first point in this line as last and push line to poly
            points.push_back(*(points.end() - nPoints));
            nPoints++;

            auto coordIt = points.end() - nPoints;
            double area = signedArea(coordIt, coordIt + nPoints);

            if (area == 0.f) {
                // Erase empty ring
                points.erase(coordIt, coordIt + nPoints);
                nPoints = 0;
                continue;
            }

            int curWinding = area > 0 ? 1 : -1;

            if (winding == 0) {
                winding = curWinding;
            }

            if (nRings == 0) {
                // Add outer ring
                geometry.endRing();
                nRings = 1;
            } else if (winding == curWinding) {
                // End current Polygon
                geometry.endPoly();

                // This ring starts new Polygon
                geometry.endRing();
                nRings = 1;
            } else {
                // Add inner ring
                geometry.endRing();
                nRings++;
            }

            // Enforce consistent winding
            if (curWinding < 0) {
                std::reverse(coordIt, coordIt + nPoints);
            }

            nPoints = 0;
        }
    }

    assert(_geomIn.getData() == _geomIn.getEnd());

    if (nPoints > 0) {
        // End the last line
        assert(geometry.type == GeometryType::lines ||
               geometry.type == GeometryType::points);
        geometry.endLine();
    }

    if (nRings > 0) {
        // End last Polygon - this case may only happen when the last ring had zero area!
        assert(geometry.type == GeometryType::polygons);
        geometry.endPoly();
    }
}

// This is a bit heavy machinery to reuse string allocations in variants
struct update_visitor {
    using result_type = bool;
    const PbfParser::TagValue& tagValue;
    Value& propValue;

    struct double_visitor {
        using result_type = bool;
        Value& propValue;

        bool operator()(const PbfParser::StringView& v) const {
            // Create new string from StringView
            propValue = std::string(v.second, v.first);
            return false;
        }
        template<typename T>
        bool operator()(const T& v) const {
            propValue = v;
            return true;
        }
    };

    struct string_visitor {
        using result_type = bool;
        Value& propValue;
        std::string& str;

        bool operator()(const PbfParser::StringView& v) const {
            // Assign StringView to previous string
            str.assign(v.second, v.first);
            return true;
        }
        template<typename T>
        bool operator()(const T& v) const {
            propValue = v;
            return false;
        }
    };

    template<typename T>
    bool operator()(T& v) const {
        // propValue was not string, replace it with new tagValue
        return PbfParser::TagValue::visit(tagValue, double_visitor{propValue});
    }

    bool operator()(std::string& str) const {
        // propValue was string, replace it with new tagValue.
        // Reuses alloction when new value is also string.
        return PbfParser::TagValue::visit(tagValue, string_visitor{propValue, str});
    }
};

bool PbfParser::extractTags(ParserContext& _ctx, protobuf::message& _tagsMsg) {

    // keep previous tags to check which tags changed
    _ctx.featureTags.swap(_ctx.previousTags);

    // clear current tags
    _ctx.featureTags.assign(_ctx.keys.size(), -1);

    size_t numTags = 0;

    while(_tagsMsg) {
        auto tagKey = _tagsMsg.varint();

        if(_ctx.keys.size() <= tagKey) {
            LOGE("accessing out of bound key");
            return false;
        }

        if(!_tagsMsg) {
            LOGE("uneven number of feature tag ids");
            return false;
        }

        uint64_t valueKey = _tagsMsg.varint();

        if(_ctx.values.size() <= valueKey ) {
            LOGE("accessing out of bound values");
            return false;
        }

        _ctx.featureTags[tagKey] = valueKey;
        numTags++;
    }

    auto& properties = _ctx.feature.props.items();
    properties.resize(numTags);

    // Only update keys and values that differ from last feature.
    bool matchingKeys = true;
    numTags = 0;

    for (int tagKey : _ctx.orderedKeys) {
        int tagValue = _ctx.featureTags[tagKey];

        if (matchingKeys) {
            int prevTagValue = _ctx.previousTags[tagKey];

            if (tagValue < 0) {
                if (prevTagValue >= 0) {
                    // Properties key-set changed from last feature
                    matchingKeys = false;
                }
                continue;
            }

            if (prevTagValue < 0) {
                // Previous feature did not have this key
                matchingKeys = false;

            } else if (prevTagValue == tagValue) {
                numTags++;
                // Same tag as the previous feature
                continue;
            }
        } else if (tagValue < 0) { continue; }

        auto& prop = properties[numTags++];

        if (!matchingKeys) {
            prop.key = _ctx.keys[tagKey];
        }

        // Update value, reuse string allocation if possible
        Value::visit(prop.value, update_visitor{_ctx.values[tagValue], prop.value});

    }

    return true;
}

void PbfParser::extractFeature(ParserContext& _ctx, protobuf::message& _featureIn,
                               TileDataSink& _sink) {

    protobuf::message geomMsg;
    protobuf::message tagsMsg;

    auto& feature = _ctx.feature;

    while(_featureIn.next()) {
        switch(_featureIn.tag) {
            case FEATURE_ID:
                // ignored for now, also not used in json parsing
                _featureIn.skip();
                break;

            case FEATURE_TAGS:
                tagsMsg = _featureIn.getMessage();
                break;

            case FEATURE_TYPE:
                feature.geometry.type = (GeometryType)_featureIn.varint();
                break;

            case FEATURE_GEOM:
                geomMsg = _featureIn.getMessage();
                break;

            default:
                _featureIn.skip();
                break;
        }
    }

    if (!geomMsg || feature.geometry.type == GeometryType::unknown) {
        return;
    }

    if (tagsMsg && !extractTags(_ctx, tagsMsg)) {
        return;
    }

    if (!_sink.matchFeature(feature)) {
        return;
    }

    extractGeometry(_ctx, geomMsg);

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
