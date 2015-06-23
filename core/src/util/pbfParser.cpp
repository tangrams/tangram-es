#include "pbfParser.h"
#include "platform.h"

#include <cmath>


void PbfParser::extractGeometry(protobuf::message& _geomIn, int _tileExtent, std::vector<Line>& _out, const MapTile& _tile) {
    
    pbfGeomCmd cmd = pbfGeomCmd::moveTo;
    uint32_t cmdRepeat = 0;
    
    double invTileExtent = (1.0/(double)_tileExtent);
    
    Line line;
    
    int64_t x = 0;
    int64_t y = 0;
    
    while(_geomIn.getData() < _geomIn.getEnd()) {
        
        if(cmdRepeat == 0) { // get new command, lengh and parameters..
            uint32_t cmdData = static_cast<uint32_t>(_geomIn.varint());
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
            
            x += _geomIn.svarint();
            y += _geomIn.svarint();
            
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

void PbfParser::extractFeature(protobuf::message& _featureIn, Feature& _out, const MapTile& _tile, std::vector<std::string>& _keys, std::vector<float>& _numericValues, std::vector<std::string>& _stringValues, int _tileExtent) {

    //Iterate through this feature
    std::vector<Line> geometryLines;
    protobuf::message geometry; // By default data_ and end_ are nullptr
    
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
                    
                    const std::string& key = _keys[tagKey];
                    const std::string& strVal = _stringValues[valueKey];
                    float numVal = _numericValues[valueKey];
                    
                    if(!isnan(numVal)) {
                        
                        // height and minheight need to be handled separately so that their dimensions are normalized
                        if(key.compare("height") == 0 || key.compare("min_height") == 0) {
                            numVal *= _tile.getInverseScale();
                        }
                        _out.props.numericProps.emplace(key, numVal);
                        
                    } else {
                        
                        _out.props.stringProps.emplace(key, strVal);
                        
                    }
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
                extractGeometry(geometry, _tileExtent, geometryLines, _tile);
                break;
            // None.. skip
            default:
                _featureIn.skip();
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

void PbfParser::extractLayer(protobuf::message& _layerIn, Layer& _out, const MapTile& _tile) {
    
    std::vector<std::string> keys;
    std::vector<float> numericValues;
    std::vector<std::string> stringValues;
    std::vector<protobuf::message> featureMsgs;
    int tileExtent = 0;
    
    //iterate layer to populate featureMsgs, keys and values
    while(_layerIn.next()) {
        switch(_layerIn.tag) {
            case 2: // features
            {
                featureMsgs.push_back(_layerIn.getMessage());
                break;
            }
                
            case 3: // key string
            {
                keys.push_back(_layerIn.string());
                break;
            }

            case 4: // values
            {
                protobuf::message valueItr = _layerIn.getMessage();
                
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
                tileExtent = static_cast<int>(_layerIn.int64());
                break;
                
            default: // skip
                _layerIn.skip();
                break;
                
        }
    }
    
    for(auto& featureMsg : featureMsgs) {
        _out.features.emplace_back();
        extractFeature(featureMsg, _out.features.back(), _tile, keys, numericValues, stringValues, tileExtent);
    }
}
