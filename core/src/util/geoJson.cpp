#include "geoJson.h"
#include "platform.h"
#include "util/mapProjection.h"

void GeoJson::extractPoint(const rapidjson::Value& _in, Point& _out, const MapTile& _tile) {
    
    glm::dvec2 tmp = _tile.getProjection()->LonLatToMeters(glm::dvec2(_in[0].GetDouble(), _in[1].GetDouble()));
    _out.x = (tmp.x - _tile.getOrigin().x) * _tile.getInverseScale();
    _out.y = (tmp.y - _tile.getOrigin().y) * _tile.getInverseScale();
    
}

void GeoJson::extractLine(const rapidjson::Value& _in, Line& _out, const MapTile& _tile) {
    
    for (auto itr = _in.Begin(); itr != _in.End(); ++itr) {
        _out.emplace_back();
        extractPoint(*itr, _out.back(), _tile);
    }
    
}

void GeoJson::extractPoly(const rapidjson::Value& _in, Polygon& _out, const MapTile& _tile) {
    
    for (auto itr = _in.Begin(); itr != _in.End(); ++itr) {
        _out.emplace_back();
        extractLine(*itr, _out.back(), _tile);
    }
    
}

void GeoJson::extractFeature(const rapidjson::Value& _in, Feature& _out, const MapTile& _tile) {
    // Copy properties into tile data
    
    const rapidjson::Value& properties = _in["properties"];
    
    for (auto itr = properties.MemberBegin(); itr != properties.MemberEnd(); ++itr) {
        
        const auto& member = itr->name.GetString();
        
        const rapidjson::Value& prop = properties[member];
        
        // height and minheight need to be handled separately so that their dimensions are normalized
        if (strcmp(member, "height") == 0) {
            _out.props.emplace(member, prop.GetDouble() * _tile.getInverseScale());
            continue;
        }
        
        if (strcmp(member, "min_height") == 0) {
            _out.props.emplace(member, prop.GetDouble() * _tile.getInverseScale());
            continue;
        }
        
        
        if (prop.IsNumber()) {
            _out.props.emplace(member, prop.GetDouble());
        } else if (prop.IsString()) {
            _out.props.emplace(member, prop.GetString());
        }
        
    }
    
    // Copy geometry into tile data
    
    const rapidjson::Value& geometry = _in["geometry"];
    const rapidjson::Value& coords = geometry["coordinates"];
    const std::string& geometryType = geometry["type"].GetString();
    
    if (geometryType.compare("Point") == 0) {
        
        _out.geometryType = GeometryType::POINTS;
        _out.points.emplace_back();
        extractPoint(coords, _out.points.back(), _tile);
        
    } else if (geometryType.compare("MultiPoint") == 0) {
        
        _out.geometryType= GeometryType::POINTS;
        for (auto pointCoords = coords.Begin(); pointCoords != coords.End(); ++pointCoords) {
            _out.points.emplace_back();
            extractPoint(*pointCoords, _out.points.back(), _tile);
        }
        
    } else if (geometryType.compare("LineString") == 0) {
        _out.geometryType = GeometryType::LINES;
        _out.lines.emplace_back();
        extractLine(coords, _out.lines.back(), _tile);
        
    } else if (geometryType.compare("MultiLineString") == 0) {
        _out.geometryType = GeometryType::LINES;
        for (auto lineCoords = coords.Begin(); lineCoords != coords.End(); ++lineCoords) {
            _out.lines.emplace_back();
            extractLine(*lineCoords, _out.lines.back(), _tile);
        }
        
    } else if (geometryType.compare("Polygon") == 0) {
        
        _out.geometryType = GeometryType::POLYGONS;
        _out.polygons.emplace_back();
        extractPoly(coords, _out.polygons.back(), _tile);
        
    } else if (geometryType.compare("MultiPolygon") == 0) {
        
        _out.geometryType = GeometryType::POLYGONS;
        for (auto polyCoords = coords.Begin(); polyCoords != coords.End(); ++polyCoords) {
            _out.polygons.emplace_back();
            extractPoly(*polyCoords, _out.polygons.back(), _tile);
        }
        
    }
}

void GeoJson::extractLayer(const rapidjson::Value& _in, Layer& _out, const MapTile& _tile) {
    
    const auto& featureIter = _in.FindMember("features");
    
    if (featureIter == _in.MemberEnd()) {
        logMsg("ERROR: GeoJSON missing 'features' member\n");
        return;
    }
    
    const auto& features = featureIter->value;
    for (auto featureJson = features.Begin(); featureJson != features.End(); ++featureJson) {
        _out.features.emplace_back();
        extractFeature(*featureJson, _out.features.back(), _tile);
    }
    
}
