#include "pbfParser.h"
#include "platform.h"


void PBFHelper::extractMessage(protobuf::message& _in, protobuf::message& _out) {
    uint64_t len = _in.varint();
    _out = protobuf::message _in.getData(),static_cast<std::size_t>(len));
}

void PbfParser::extractPoint(const protobuf::message& _in, Point& _out, const MapTile& _tile) {
    
    glm::dvec2 tmp = _tile.getProjection()->LonLatToMeters(glm::dvec2(_in[0].asDouble(), _in[1].asDouble()));
    _out.x = (tmp.x - _tile.getOrigin().x) * _tile.getInverseScale();
    _out.y = (tmp.y - _tile.getOrigin().y) * _tile.getInverseScale();
    
}

void PbfParser::extractLine(const protobuf::message& _in, Line& _out, const MapTile& _tile) {
    
    for (auto& point : _in) {
        _out.emplace_back();
        extractPoint(point, _out.back(), _tile);
    }
    
}

void PbfParser::extractPoly(const protobuf::message& _in, Polygon& _out, const MapTile& _tile) {
    
    for (auto& line : _in) {
        _out.emplace_back();
        extractLine(line, _out.back(), _tile);
    }
    
}

void PbfParser::extractFeature(const protobuf::message& _in, Feature& _out, const MapTile& _tile, std::vector<std::string>& _keys, std::unordered_map<int, float>& _numericValues, std::unordered_map<int, std::string>& _stringValues) {

    //Iterate through this feature
    protobuf::message featureItr = _in;
    while(featureItr.next()) {
        switch(featureItr.tag) {
            // Feature ID
            case 1:
                // ignored for now, also not used in json parsing
                break;
            // Feature tags (properties)
            case 2:
                // extract tags message
                protobuf::message tagsMsg = featureItr.getMessage();
                
                while(tagsMsg) {
                    std::size_t tagKey = tagsMsg.varint();
                    
                    if(_keys.size() < tagKey) {
                        logMsg("ERROR: accessing out of bound key\n");
                    }
                    
                    if(tagsMsg) {
                        std::size_t valueKey = tagsMsg.varint();
                        
                        if( (_numericValues.size() + _stringValues.size()) < valueKey ) {
                            logMsg("ERROR: accessing out of bound values\n");
                        }
                        
                        if(_numericValues.find(valueKey) != _numericValues.end()) {
                            _out.props.numericProps[_keys[tagKey]] = _numericValues[valueKey];
                        } else if(_stringValues.find(valueKey) != _stringValues.end()) {
                            _out.props.stringProps[_keys[tagKey]] = _stringValues[valueKey];
                        } else {
                            logMsg(")
                        }
                        
                    }
                }
                
                break;
            // Feature Type
            case 3:
                break;
            // Actual geometry data
            case 4:
                break;
            // None.. skip
            default:
                break;
        }
    }
    
    // Copy properties into tile data
    
    const Json::Value& properties = _in["properties"];
    
    for (auto& member : properties.getMemberNames()) {
        
        const Json::Value& prop = properties[member];
        
        // height and minheight need to be handled separately so that their dimensions are normalized
        if (member.compare("height") == 0) {
            _out.props.numericProps[member] = prop.asFloat() * _tile.getInverseScale();
            continue;
        }
        
        if (member.compare("min_height") == 0) {
            _out.props.numericProps[member] = prop.asFloat() * _tile.getInverseScale();
            continue;
        }
        
        
        if (prop.isNumeric()) {
            _out.props.numericProps[member] = prop.asFloat();
        } else if (prop.isString()) {
            _out.props.stringProps[member] = prop.asString();
        }
        
    }
    
    // Copy geometry into tile data
    
    const Json::Value& geometry = _in["geometry"];
    const Json::Value& coords = geometry["coordinates"];
    const std::string& geometryType = geometry["type"].asString();
    
    if (geometryType.compare("Point") == 0) {
        
        _out.geometryType = GeometryType::POINTS;
        _out.points.emplace_back();
        extractPoint(coords, _out.points.back(), _tile);
        
    } else if (geometryType.compare("MultiPoint") == 0) {
        
        _out.geometryType= GeometryType::POINTS;
        for (const auto& pointCoords : coords) {
            _out.points.emplace_back();
            extractPoint(pointCoords, _out.points.back(), _tile);
        }
        
    } else if (geometryType.compare("LineString") == 0) {
        _out.geometryType = GeometryType::LINES;
        _out.lines.emplace_back();
        extractLine(coords, _out.lines.back(), _tile);
        
    } else if (geometryType.compare("MultiLineString") == 0) {
        _out.geometryType = GeometryType::LINES;
        for (const auto& lineCoords : coords) {
            _out.lines.emplace_back();
            extractLine(lineCoords, _out.lines.back(), _tile);
        }
        
    } else if (geometryType.compare("Polygon") == 0) {
        
        _out.geometryType = GeometryType::POLYGONS;
        _out.polygons.emplace_back();
        extractPoly(coords, _out.polygons.back(), _tile);
        
    } else if (geometryType.compare("MultiPolygon") == 0) {
        
        _out.geometryType = GeometryType::POLYGONS;
        for (const auto& polyCoords : coords) {
            _out.polygons.emplace_back();
            extractPoly(polyCoords, _out.polygons.back(), _tile);
        }
        
    }
    
}

void PbfParser::extractLayer(const protobuf::message& _in, Layer& _out, const MapTile& _tile) {

    protobuf::message layerItr = _in;
    
    std::vector<std::string> keys;
    std::unordered_map<int, float> numericValues;
    std::unordered_map<int, std::string> stringValues;
    
    //iterate layer to populate keys and values
    int valueCount = 0;
    while(layerItr.next()) {
        switch(layerItr.tag) {
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
        
            default: // skip
                layerItr.skip();
                break;
                
        }
    }
    
    //reset layer iterator
    layerItr = _in;
    
    // iterate layer to extract features
    while(layerItr.next()) {
        if(layerItr.tag == 2) {
            protobuf::message featureMsg = layerItr.getMessage();
            _out.features.emplace_back();
            extractFeature(featureMsg, _out.features.back(), _tile, keys, numericValues, stringValues);
        } else {
            layerItr.skip();
        }
    }
}
