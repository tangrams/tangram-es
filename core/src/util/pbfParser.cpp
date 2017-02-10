#include "util/pbfParser.h"

#include "data/propertyItem.h"
#include "tile/tile.h"
#include "log.h"
#include "platform.h"
#include "util/geom.h"

#include <algorithm>
#include <iterator>

#define FEATURE_ID 1
#define FEATURE_TAGS 2
#define FEATURE_TYPE 3
#define FEATURE_GEOM 4


#define LAYER_NAME 1
#define LAYER_FEATURE 2
#define LAYER_KEY 3
#define LAYER_VALUE 4
#define LAYER_TILE_EXTENT 5

namespace Tangram {

PbfParser::Geometry PbfParser::getGeometry(ParserContext& _ctx, protobuf::message _geomIn) {

    Geometry geometry;

    pbfGeomCmd cmd = pbfGeomCmd::moveTo;
    uint32_t cmdRepeat = 0;

    double invTileExtent = (1.0/(_ctx.tileExtent-1.0));

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
                if (geometry.coordinates.size() > 0) {
                    geometry.sizes.push_back(numCoordinates);
                }
                numCoordinates = 0;
            }

            x += _geomIn.svarint();
            y += _geomIn.svarint();

            // bring the points in 0 to 1 space
            Point p;
            p.x = invTileExtent * (double)x;
            p.y = invTileExtent * (double)(_ctx.tileExtent - y);

            if (numCoordinates == 0 || geometry.coordinates.back() != p) {
                geometry.coordinates.push_back(p);
                numCoordinates++;
            }
        } else if(cmd == pbfGeomCmd::closePath) {
            // end of a polygon, push first point in this line as last and push line to poly
            geometry.coordinates.push_back(geometry.coordinates[geometry.coordinates.size() - numCoordinates]);
            geometry.sizes.push_back(numCoordinates + 1);
            numCoordinates = 0;
        }

        cmdRepeat--;
    }

    // Enter the last line
    if (numCoordinates > 0) {
        geometry.sizes.push_back(numCoordinates);
    }

    return geometry;
}

Feature PbfParser::getFeature(ParserContext& _ctx, protobuf::message _featureIn) {

    Feature feature(_ctx.sourceId);

    _ctx.featureTags.clear();
    _ctx.featureTags.assign(_ctx.keys.size(), -1);


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
                        return feature;
                    }

                    if(!tagsMsg) {
                        LOGE("uneven number of feature tag ids");
                        return feature;
                    }

                    auto valueKey = tagsMsg.varint();

                    if( _ctx.values.size() <= valueKey ) {
                        LOGE("accessing out of bound values");
                        return feature;
                    }

                    _ctx.featureTags[tagKey] = valueKey;
                }
                break;
            }
            case FEATURE_TYPE:
                feature.geometryType = (GeometryType)_featureIn.varint();
                break;
            // Actual geometry data
            case FEATURE_GEOM:
                _ctx.geometry = getGeometry(_ctx, _featureIn.getMessage());
                break;

            default:
                _featureIn.skip();
                break;
        }
    }

    std::vector<Properties::Item> properties;
    properties.reserve(_ctx.featureTags.size());

    for (int tagKey : _ctx.orderedKeys) {
        int tagValue = _ctx.featureTags[tagKey];
        if (tagValue >= 0) {
            properties.emplace_back(_ctx.keys[tagKey], _ctx.values[tagValue]);
        }
    }
    feature.props.setSorted(std::move(properties));

    switch(feature.geometryType) {
        case GeometryType::points:
            feature.points.insert(feature.points.begin(),
                                  _ctx.geometry.coordinates.begin(),
                                  _ctx.geometry.coordinates.end());
            break;

        case GeometryType::lines:
        {
            auto pos = _ctx.geometry.coordinates.begin();
            for (int length : _ctx.geometry.sizes) {
                if (length == 0) { continue; }
                Line line;
                line.reserve(length);
                line.insert(line.begin(), pos, pos + length);
                pos += length;
                feature.lines.emplace_back(std::move(line));
            }
            break;
        }
        case GeometryType::polygons:
        {
            auto pos = _ctx.geometry.coordinates.begin();
            auto rpos = _ctx.geometry.coordinates.rend();
            for (int length : _ctx.geometry.sizes) {
                if (length == 0) { continue; }
                float area = signedArea(pos, pos + length);
                if (area == 0) {
                    pos += length;
                    rpos -= length;
                    continue;
                }
                int winding = area > 0 ? 1 : -1;
                // Determine exterior winding from first polygon.
                if (_ctx.winding == 0) {
                    _ctx.winding = winding;
                }
                Line line;
                line.reserve(length);
                if (_ctx.winding > 0) {
                    line.insert(line.end(), pos, pos + length);
                } else {
                    line.insert(line.end(), rpos - length, rpos);
                }
                pos += length;
                rpos -= length;
                if (winding == _ctx.winding || feature.polygons.empty()) {
                    // This is an exterior polygon.
                    feature.polygons.emplace_back();
                }
                feature.polygons.back().push_back(std::move(line));
            }
            break;
        }
        case GeometryType::unknown:
            break;
        default:
            break;
    }

    return feature;
}

Layer PbfParser::getLayer(ParserContext& _ctx, protobuf::message _layerIn) {

    Layer layer("");

    _ctx.keys.clear();
    _ctx.values.clear();
    _ctx.featureMsgs.clear();

    bool lastWasFeature = false;
    size_t numFeatures = 0;
    protobuf::message featureItr;

    // Iterate layer to populate featureMsgs, keys and values
    while(_layerIn.next()) {

        switch(_layerIn.tag) {
            case LAYER_NAME: {
                layer.name = _layerIn.string();
                break;
            }
            case LAYER_FEATURE: {
                numFeatures++;
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
        lastWasFeature = false;
    }

    if (_ctx.featureMsgs.empty()) { return layer; }

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

    layer.features.reserve(numFeatures);
    for (auto& featureItr : _ctx.featureMsgs) {
        do {
            auto featureMsg = featureItr.getMessage();

            layer.features.push_back(getFeature(_ctx, featureMsg));

        } while (featureItr.next() && featureItr.tag == LAYER_FEATURE);
    }

    return layer;
}

}
