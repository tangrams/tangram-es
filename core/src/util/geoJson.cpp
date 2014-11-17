#include "geoJson.h"
#include "platform.h"

void GeoJson::extractPoint(const Json::Value& _in, Point& _out, const MapTile& _tile) {
    
    glm::dvec2 tmp = _tile.getProjection()->LonLatToMeters(glm::dvec2(_in[0].asDouble(), _in[1].asDouble()));
    _out.x = (tmp.x - _tile.getOrigin().x) * _tile.getInverseScale();
    _out.y = (tmp.y - _tile.getOrigin().y) * _tile.getInverseScale();
    
}

void GeoJson::extractLine(const Json::Value& _in, Line& _out, const MapTile& _tile) {
    
    for (auto& point : _in) {
        _out.emplace_back();
        extractPoint(point, _out.back(), _tile);
    }
    
}

void GeoJson::extractPoly(const Json::Value& _in, Polygon& _out, const MapTile& _tile) {
    
    for (auto& line : _in) {
        _out.emplace_back();
        extractLine(line, _out.back(), _tile);
    }
    
}

void GeoJson::extractFeature(const Json::Value& _in, Feature& _out, const MapTile& _tile) {
    
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
        
    } else if (geometryType.compare("Line") == 0) {
        logMsg("\t\t***Testing\n");
        _out.geometryType = GeometryType::LINES;
        _out.lines.emplace_back();
        extractLine(coords, _out.lines.back(), _tile);
        
    } else if (geometryType.compare("MultiLine") == 0) {
        logMsg("\t\t***Testing\n");
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

void GeoJson::extractLayer(const Json::Value& _in, Layer& _out, const MapTile& _tile) {
    
    const Json::Value& features = _in["features"];
    
    for (const auto& featureJson : features) {
        _out.features.emplace_back();
        extractFeature(featureJson, _out.features.back(), _tile);
    }
    
}
