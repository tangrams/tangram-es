#include "pbfParser.h"
#include "platform.h"

#include <cmath>


void PbfParser::extractGeometry(protobuf::message& _in, int _tileExtent, std::vector<Line>& _out, const MapTile& _tile) {
    
    pbfGeomCmd cmd = pbfGeomCmd::moveTo;
    uint32_t cmdRepeat = 0;
    protobuf::message& geomItr = _in;
    
    double invTileExtent = (1.0/(double)_tileExtent);
    
    Line line;
    
    int64_t x = 0;
    int64_t y = 0;
    
    while(geomItr.getData() < geomItr.getEnd()) {
        
        if(cmdRepeat == 0) { // get new command, lengh and parameters..
            uint32_t cmdData = static_cast<uint32_t>(geomItr.varint());
            cmd = static_cast<pbfGeomCmd>(cmdData & 0x7); //first 3 bits of the cmdData
            cmdRepeat = cmdData >> 3; //last 5 bits
        }
        
        if(cmd == pbfGeomCmd::moveTo || cmd == pbfGeomCmd::lineTo) { // get parameters/points
            // if cmd is move then move to a new line/set of points and save this line
            if(cmd == pbfGeomCmd::moveTo) {
                if(line.size() > 0) {
                    _out.push_back(line);
                }
                line.clear();
            }
            
            x += geomItr.svarint();
            y += geomItr.svarint();
            
            // bring the points in -1 to 1 space
            Point p;
            p.x = invTileExtent * (double)(2 * x - _tileExtent);
            p.y = invTileExtent * (double)(_tileExtent - 2 * y);
            
            line.push_back(p);
            
        } else if( cmd == pbfGeomCmd::closePath) { // end of a polygon, push first point in this line as last and push line to poly
            line.push_back(line[0]);
            _out.push_back(line);
            line.clear();
        }
        
        cmdRepeat--;
    }
    
    // Enter the last line
    if(line.size() > 0) {
        _out.push_back(line);
    }
    
}

void PbfParser::extractFeature(protobuf::message& _in, Feature& _out, const MapTile& _tile, std::vector<std::string>& _keys, std::vector<float>& _numericValues, std::vector<std::string>& _stringValues, int _tileExtent) {

    //Iterate through this feature
    std::vector<Line> geometryLines;
    protobuf::message featureItr = _in;
    protobuf::message geometry; // By default data_ and end_ are nullptr
    
    while(featureItr.next()) {
        switch(featureItr.tag) {
            // Feature ID
            case 1:
                // ignored for now, also not used in json parsing
                featureItr.skip();
                break;
            // Feature tags (properties)
            case 2:
            {
                // extract tags message
                protobuf::message tagsMsg = featureItr.getMessage();
                
                while(tagsMsg) {
                    std::size_t tagKey = tagsMsg.varint();
                    
                    if(_keys.size() <= tagKey) {
                        logMsg("ERROR: accessing out of bound key\n");
                        return;
                    }
                    
                    if(!tagsMsg) {
                        logMsg("ERROR: uneven number of feature tag ids\n");
                        return;
                    }
                    
                    std::size_t valueKey = tagsMsg.varint();
                    
                    if( _numericValues.size() <= valueKey ) {
                        logMsg("ERROR: accessing out of bound values\n");
                        return;
                    }
                    
                    const auto& key = _keys[tagKey];
                    float numVal = _numericValues[valueKey];
                    const auto& strVal = _stringValues[valueKey];
                    
                    if(numVal != NAN) {
                        
                        // height and minheight need to be handled separately so that their dimensions are normalized
                        if(key.compare("height") == 0 || key.compare("min_height") == 0) {
                            numVal *= _tile.getInverseScale();
                        }
                        _out.props.numericProps[key] = numVal;
                        
                    } else if (!strVal.empty()) {
                        
                        _out.props.stringProps[key] = strVal;
                        
                    } else {
                        
                        logMsg("ERROR: tag missing\n");
                        
                    }
                }
                break;
            }
            // Feature Type
            case 3:
                _out.geometryType = (GeometryType)featureItr.varint();
                break;
            // Actual geometry data
            case 4:
                geometry = featureItr.getMessage();
                extractGeometry(geometry, _tileExtent, geometryLines, _tile);
                break;
            // None.. skip
            default:
                featureItr.skip();
                break;
        }
    }
    
    switch(_out.geometryType) {
        case GeometryType::POINTS:
            for(auto& line : geometryLines) {
                for(auto& point : line) {
                    _out.points.emplace_back(point);
                }
            }
            break;
        case GeometryType::LINES:
            for(auto& line : geometryLines) {
                _out.lines.emplace_back(line);
            }
            break;
        case GeometryType::POLYGONS:
            _out.polygons.emplace_back(geometryLines);
            break;
        case GeometryType::UNKNOWN:
            break;
        default:
            break;
    }
    
}

void PbfParser::extractLayer(protobuf::message& _in, Layer& _out, const MapTile& _tile) {

    protobuf::message& layerItr = _in;
    
    std::vector<std::string> keys;
    std::vector<float> numericValues;
    std::vector<std::string> stringValues;
    std::vector<protobuf::message> featureMsgs;
    int tileExtent = 0;
    
    //iterate layer to populate featureMsgs, keys and values
    while(layerItr.next()) {
        switch(layerItr.tag) {
            case 2: // features
            {
                featureMsgs.push_back(layerItr.getMessage());
                break;
            }
                
            case 3: // key string
            {
                keys.push_back(layerItr.string());
                break;
            }

            case 4: // values
            {
                protobuf::message valueItr = layerItr.getMessage();
                
                while (valueItr.next()) {
                    switch (valueItr.tag) {
                        case 1: // string value
                            stringValues.push_back(valueItr.string());
                            numericValues.push_back(NAN);
                            break;
                        case 2: // float value
                            numericValues.push_back(valueItr.float32());
                            stringValues.push_back("");
                            break;
                        case 3: // double value
                            numericValues.push_back(valueItr.float64());
                            stringValues.push_back("");
                            break;
                        case 4: // int value
                            numericValues.push_back(valueItr.int64());
                            stringValues.push_back("");
                            break;
                        case 5: // uint value
                            numericValues.push_back(valueItr.varint());
                            stringValues.push_back("");
                            break;
                        case 6: // sint value
                            numericValues.push_back(valueItr.int64());
                            stringValues.push_back("");
                            break;
                        case 7: // bool value
                            numericValues.push_back(valueItr.boolean());
                            stringValues.push_back("");
                            break;
                        default:
                            numericValues.push_back(NAN);
                            stringValues.push_back("");
                            valueItr.skip();
                            break;
                    }
                }
                break;
            }
                
            case 5: //extent
                tileExtent = static_cast<int>(layerItr.int64());
                break;
                
            default: // skip
                layerItr.skip();
                break;
                
        }
    }
    
    for(auto& featureMsg : featureMsgs) {
        _out.features.emplace_back();
        extractFeature(featureMsg, _out.features.back(), _tile, keys, numericValues, stringValues, tileExtent);
    }
}
