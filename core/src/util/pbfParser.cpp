#include "pbfParser.h"

#include "tile/tile.h"
#include "platform.h"

namespace Tangram {

void PbfParser::extractGeometry(ParserContext& ctx, protobuf::message& _geomIn) {

    pbfGeomCmd cmd = pbfGeomCmd::moveTo;
    uint32_t cmdRepeat = 0;

    double invTileExtent = (1.0/(double)ctx.tileExtent);

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
                if(ctx.coordinates.size() > 0) {
                    ctx.numCoordinates.push_back(numCoordinates);
                }
                numCoordinates = 0;
            }

            x += _geomIn.svarint();
            y += _geomIn.svarint();

            // bring the points in -1 to 1 space
            Point p;
            p.x = invTileExtent * (double)(2 * x - ctx.tileExtent);
            p.y = invTileExtent * (double)(ctx.tileExtent - 2 * y);

            ctx.coordinates.push_back(p);
            numCoordinates++;

        } else if(cmd == pbfGeomCmd::closePath) {
            // end of a polygon, push first point in this line as last and push line to poly
            ctx.coordinates.push_back(ctx.coordinates[ctx.coordinates.size() - numCoordinates]);
            ctx.numCoordinates.push_back(numCoordinates + 1);
            numCoordinates = 0;
        }

        cmdRepeat--;
    }

    // Enter the last line
    if (numCoordinates > 0) {
        ctx.numCoordinates.push_back(numCoordinates);
    }
}

void PbfParser::extractFeature(ParserContext& ctx, protobuf::message& _featureIn, Feature& _out) {

    //Iterate through this feature
    protobuf::message geometry; // By default data_ and end_ are nullptr

    ctx.properties.clear();
    ctx.coordinates.clear();
    ctx.numCoordinates.clear();

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
                    std::size_t tagKey = tagsMsg.varint();

                    if(ctx.keys.size() <= tagKey) {
                        logMsg("ERROR: accessing out of bound key\n");
                        return;
                    }

                    if(!tagsMsg) {
                        logMsg("ERROR: uneven number of feature tag ids\n");
                        return;
                    }

                    std::size_t valueKey = tagsMsg.varint();

                    if( ctx.values.size() <= valueKey ) {
                        logMsg("ERROR: accessing out of bound values\n");
                        return;
                    }

                    ctx.properties.emplace_back(ctx.keys[tagKey], ctx.values[valueKey]);
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
                extractGeometry(ctx, geometry);
                break;
            // None.. skip
            default:
                _featureIn.skip();
                break;
        }
    }
    _out.props = std::move(ctx.properties);


    switch(_out.geometryType) {
        case GeometryType::points:
            _out.points.insert(_out.points.begin(),
                               ctx.coordinates.begin(),
                               ctx.coordinates.end());
            break;

        case GeometryType::lines:
        case GeometryType::polygons:
        {
            std::vector<Line> lines;
            int offset = 0;
            lines.reserve(ctx.numCoordinates.size());

            for (int length : ctx.numCoordinates) {
                if (length == 0) {
                    continue;
                }

                lines.emplace_back();
                auto& line = lines.back();
                line.reserve(length);

                line.insert(line.begin(),
                            ctx.coordinates.begin() + offset,
                            ctx.coordinates.begin() + offset + length);

                offset += length;
            }
            if (_out.geometryType == GeometryType::lines) {
                _out.lines = std::move(lines);
            } else {
                _out.polygons.push_back(std::move(lines));
            }
            break;
        }
        case GeometryType::unknown:
            break;
        default:
            break;
    }

}

void PbfParser::extractLayer(ParserContext& ctx, protobuf::message& _layerIn, Layer& _out) {

    ctx.keys.clear();
    ctx.values.clear();
    ctx.featureMsgs.clear();

    //iterate layer to populate featureMsgs, keys and values
    while(_layerIn.next()) {
        switch(_layerIn.tag) {
            case 2: // features
            {
                ctx.featureMsgs.push_back(_layerIn.getMessage());
                break;
            }

            case 3: // key string
            {
                ctx.keys.push_back(_layerIn.string());
                break;
            }

            case 4: // values
            {
                protobuf::message valueItr = _layerIn.getMessage();

                while (valueItr.next()) {
                    switch (valueItr.tag) {
                        case 1: // string value
                            ctx.values.push_back(valueItr.string());
                            break;
                        case 2: // float value
                            ctx.values.push_back(valueItr.float32());
                            break;
                        case 3: // double value
                            ctx.values.push_back(valueItr.float64());
                            break;
                        case 4: // int value
                            ctx.values.push_back(valueItr.int64());
                            break;
                        case 5: // uint value
                            ctx.values.push_back(valueItr.varint());
                            break;
                        case 6: // sint value
                            ctx.values.push_back(valueItr.int64());
                            break;
                        case 7: // bool value
                            ctx.values.push_back(valueItr.boolean());
                            break;
                        default:
                            ctx.values.push_back(none_type{});
                            valueItr.skip();
                            break;
                    }
                }
                break;
            }

            case 5: //extent
                ctx.tileExtent = static_cast<int>(_layerIn.int64());
                break;

            default: // skip
                _layerIn.skip();
                break;
        }
    }

    for(auto& featureMsg : ctx.featureMsgs) {
        _out.features.emplace_back();
        extractFeature(ctx, featureMsg, _out.features.back());
    }
}

}
