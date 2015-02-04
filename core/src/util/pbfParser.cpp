#include "pbfParser.h"
#include "platform.h"


void PbfParser::extractGeometry(const protobuf::message& _in, int _tileExtent, std::vector<Line>& _out, const MapTile& _tile) {
    
    pbfGeomCmd cmd = pbfGeomCmd::moveTo;
    uint32_t cmdRepeat = 0;
    protobuf::message geomItr = _in;
    
    double invTileExtent = (1.0/(double)_tileExtent);
    
    std::vector<Point> line;
    
    int x = 0.0f;
    int y = 0.0f;
    
    uint64_t xZigZag = 0;
    uint64_t yZigZag = 0;
    
    
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
                    _out.emplace_back(line);
                }
                line.clear();
            }
            
            // Decode zig-zag encoded paramters
            
            xZigZag += geomItr.svarint();
            yZigZag += geomItr.svarint();
            
            x = int((xZigZag << 1) ^ (xZigZag >> 31));
            y = int((yZigZag << 1) ^ (yZigZag >> 31));
            
            // bring the points in -1 to 1 space
            Point p;
            p.x = invTileExtent * (double)(x - _tileExtent);
            p.y = invTileExtent * (double)(_tileExtent - y);
            
            line.emplace_back(p);
        } else if( cmd == pbfGeomCmd::closePath) { // end of a polygon, push first point in this line as last and push line to poly
            line.push_back(line[0]);
            _out.emplace_back(line);
            line.clear();
        }
        
        cmdRepeat--;
    }
    
}

void PbfParser::extractFeature(const protobuf::message& _in, Feature& _out, const MapTile& _tile, std::vector<std::string>& _keys, std::unordered_map<int, float>& _numericValues, std::unordered_map<int, std::string>& _stringValues, int _tileExtent) {

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
                    }
                    
                    if(tagsMsg) {
                        std::size_t valueKey = tagsMsg.varint();
                        
                        if( (_numericValues.size() + _stringValues.size()) <= valueKey ) {
                            logMsg("ERROR: accessing out of bound values\n");
                        }
                        
                        if(_numericValues.find(valueKey) != _numericValues.end()) {
                            
                            // height and minheight need to be handled separately so that their dimensions are normalized
                            if(_keys[tagKey].compare("height") == 0 || _keys[tagKey].compare("min_height") == 0) {
                                _out.props.numericProps[_keys[tagKey]] = _numericValues[valueKey] * _tile.getInverseScale();
                            } else {
                                _out.props.numericProps[_keys[tagKey]] = _numericValues[valueKey];
                            }
                            
                        } else if(_stringValues.find(valueKey) != _stringValues.end()) {
                            _out.props.stringProps[_keys[tagKey]] = _stringValues[valueKey];
                        } else {
                            logMsg("ERROR: Tag is missing, should not be!!");
                        }
                    } else {
                        logMsg("uneven number of feature tag ids");
                        throw std::runtime_error("uneven number of feature tag ids");
                    }
                }
            }
                break;
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

void PbfParser::extractLayer(const protobuf::message& _in, Layer& _out, const MapTile& _tile) {

    protobuf::message layerItr = _in;
    
    std::vector<std::string> keys;
    std::unordered_map<int, float> numericValues;
    std::unordered_map<int, std::string> stringValues;
    std::vector<protobuf::message> featureMsgs;
    int tileExtent = 0;
    
    //iterate layer to populate featureMsgs, keys and values
    int valueCount = 0;
    while(layerItr.next()) {
        switch(layerItr.tag) {
            case 2: // features
            {
                protobuf::message featureMsg = layerItr.getMessage();
                featureMsgs.emplace_back(featureMsg);
                break;
            }
                
            case 3: // key string
                keys.emplace_back(layerItr.string());
                break;
                
            case 4: // values
            {
                protobuf::message valueMsg = layerItr.getMessage();
                protobuf::message valueItr = valueMsg;
                
                while (valueItr.next()) {
                    switch (valueItr.tag) {
                        case 1: // string value
                            stringValues[valueCount] = valueItr.string();
                            break;
                        case 2: // float value
                            numericValues[valueCount] = valueItr.float32();
                            break;
                        case 3: // double value
                            numericValues[valueCount] = valueItr.float64();
                            break;
                        case 4: // int value
                            numericValues[valueCount] = valueItr.int64();
                            break;
                        case 5: // uint value
                            numericValues[valueCount] = valueItr.varint();
                            break;
                        case 6: // sint value
                            numericValues[valueCount] = valueItr.int64();
                            break;
                        case 7: // bool value
                            numericValues[valueCount] = valueItr.boolean();
                            break;
                        default:
                            valueItr.skip();
                            break;
                    }
                }
                valueCount++;
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
