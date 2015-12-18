#include "pbfParser.h"

#include "data/propertyItem.h"
#include "tile/tile.h"
#include "platform.h"
#include "util/geom.h"

#include <algorithm>

namespace Tangram {

void PbfParser::extractGeometry(ParserContext& _ctx, protobuf::message& _geomIn) {

    pbfGeomCmd cmd = pbfGeomCmd::moveTo;
    uint32_t cmdRepeat = 0;

    double invTileExtent = (1.0/(double)_ctx.tileExtent);

    int64_t x = 0;
    int64_t y = 0;

    size_t numCoordinates = 0;

    while(_geomIn.getData() < _geomIn.getEnd()) {

        if(cmdRepeat == 0) { // get new command, lengh and parameters..
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

void PbfParser::extractFeature(ParserContext& _ctx, protobuf::message& _featureIn, Feature& _out) {

    //Iterate through this feature
    protobuf::message geometry; // By default data_ and end_ are nullptr

    _ctx.coordinates.clear();
    _ctx.numCoordinates.clear();

    _ctx.featureTags.clear();
    _ctx.featureTags.assign(_ctx.keys.size(), -1);


    while(_featureIn.next()) {
        switch(_featureIn.tag) {
            // Feature ID
            case 1:
                // ignored for now, also not used in json parsing
                _featureIn.skip();
                break;
            // Feature tags (properties)
            case 2:
            {
                // extract tags message
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
                }
                break;
            }
            // Feature Type
            case 3:
                _out.geometryType = (GeometryType)_featureIn.varint();
                break;
            // Actual geometry data
            case 4:
                geometry = _featureIn.getMessage();
                extractGeometry(_ctx, geometry);
                break;
            // None.. skip
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
    _out.props.setSorted(std::move(properties));

    switch(_out.geometryType) {
        case GeometryType::points:
            _out.points.insert(_out.points.begin(),
                               _ctx.coordinates.begin(),
                               _ctx.coordinates.end());
            break;

        case GeometryType::lines:
        case GeometryType::polygons:
        {
            int offset = 0;

            for (int length : _ctx.numCoordinates) {
                if (length == 0) { continue; }

                Line line;
                line.reserve(length);

                line.insert(line.begin(),
                            _ctx.coordinates.begin() + offset,
                            _ctx.coordinates.begin() + offset + length);

                offset += length;

                if (_out.geometryType == GeometryType::lines) {
                    _out.lines.emplace_back(std::move(line));
                } else {
                    // Polygons are in a flat list of rings, with ccw rings indicating
                    // the beginning of a new polygon
                    if (_out.polygons.empty()) {
                        _out.polygons.emplace_back();
                    } else {
                        double area = signedArea(line);
                        if (area > 0) {
                            _out.polygons.emplace_back();
                        } else if (area == 0){
                            continue;
                        }
                    }
                    _out.polygons.back().push_back(std::move(line));
                }
            }
            break;
        }
        case GeometryType::unknown:
            break;
        default:
            break;
    }

}

void PbfParser::extractLayer(ParserContext& _ctx, protobuf::message& _layerIn, Layer& _out) {

    _ctx.keys.clear();
    _ctx.values.clear();
    _ctx.featureMsgs.clear();

    //// Iterate layer to populate featureMsgs, keys and values
    while(_layerIn.next()) {
        switch(_layerIn.tag) {
            case 2: // features
            {
                _ctx.featureMsgs.push_back(_layerIn.getMessage());
                break;
            }

            case 3: // key string
            {
                _ctx.keys.push_back(_layerIn.string());
                break;
            }

            case 4: // values
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

            case 5: //extent
                _ctx.tileExtent = static_cast<int>(_layerIn.int64());
                break;

            default: // skip
                _layerIn.skip();
                break;
        }
    }

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

    _out.features.reserve(_ctx.featureMsgs.size());
    for(auto& featureMsg : _ctx.featureMsgs) {
        _out.features.emplace_back(_ctx.sourceId);
        extractFeature(_ctx, featureMsg, _out.features.back());
    }
}

}
