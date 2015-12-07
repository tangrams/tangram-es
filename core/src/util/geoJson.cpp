#include "geoJson.h"

#include "platform.h"
#include "data/propertyItem.h"
#include "glm/glm.hpp"

namespace Tangram {

void GeoJson::extractPoint(const rapidjson::Value& _in, Point& _out, const tileProjectionFn& _proj) {
    _out = _proj(glm::dvec2(_in[0].GetDouble(), _in[1].GetDouble()));
}

void GeoJson::extractLine(const rapidjson::Value& _in, Line& _out, const tileProjectionFn& _proj) {

    for (auto itr = _in.Begin(); itr != _in.End(); ++itr) {
        _out.emplace_back();
        extractPoint(*itr, _out.back(), _proj);
    }

}

void GeoJson::extractPoly(const rapidjson::Value& _in, Polygon& _out, const tileProjectionFn& _proj) {

    for (auto itr = _in.Begin(); itr != _in.End(); ++itr) {
        _out.emplace_back();
        extractLine(*itr, _out.back(), _proj);
    }

}

void GeoJson::extractFeature(const rapidjson::Value& _in, Feature& _out, const tileProjectionFn& _proj) {

    // Copy properties into tile data

    const rapidjson::Value& properties = _in["properties"];

    for (auto itr = properties.MemberBegin(); itr != properties.MemberEnd(); ++itr) {

        const auto& member = itr->name.GetString();

        const rapidjson::Value& prop = properties[member];

        if (prop.IsNumber()) {
            _out.props.add(member, prop.GetDouble());
        } else if (prop.IsString()) {
            _out.props.add(member, prop.GetString());
        }

    }

    // Copy geometry into tile data

    const rapidjson::Value& geometry = _in["geometry"];
    const rapidjson::Value& coords = geometry["coordinates"];
    const std::string& geometryType = geometry["type"].GetString();

    if (geometryType.compare("Point") == 0) {

        _out.geometryType = GeometryType::points;
        _out.points.emplace_back();
        extractPoint(coords, _out.points.back(), _proj);

    } else if (geometryType.compare("MultiPoint") == 0) {

        _out.geometryType= GeometryType::points;
        for (auto pointCoords = coords.Begin(); pointCoords != coords.End(); ++pointCoords) {
            _out.points.emplace_back();
            extractPoint(*pointCoords, _out.points.back(), _proj);
        }

    } else if (geometryType.compare("LineString") == 0) {
        _out.geometryType = GeometryType::lines;
        _out.lines.emplace_back();
        extractLine(coords, _out.lines.back(), _proj);

    } else if (geometryType.compare("MultiLineString") == 0) {
        _out.geometryType = GeometryType::lines;
        for (auto lineCoords = coords.Begin(); lineCoords != coords.End(); ++lineCoords) {
            _out.lines.emplace_back();
            extractLine(*lineCoords, _out.lines.back(), _proj);
        }

    } else if (geometryType.compare("Polygon") == 0) {

        _out.geometryType = GeometryType::polygons;
        _out.polygons.emplace_back();
        extractPoly(coords, _out.polygons.back(), _proj);

    } else if (geometryType.compare("MultiPolygon") == 0) {

        _out.geometryType = GeometryType::polygons;
        for (auto polyCoords = coords.Begin(); polyCoords != coords.End(); ++polyCoords) {
            _out.polygons.emplace_back();
            extractPoly(*polyCoords, _out.polygons.back(), _proj);
        }

    }

}

void GeoJson::extractLayer(int32_t _sourceId, const rapidjson::Value& _in, Layer& _out, const tileProjectionFn& _proj) {

    const auto& featureIter = _in.FindMember("features");

    if (featureIter == _in.MemberEnd()) {
        LOGE("GeoJSON missing 'features' member");
        return;
    }

    const auto& features = featureIter->value;
    for (auto featureJson = features.Begin(); featureJson != features.End(); ++featureJson) {
        _out.features.emplace_back(_sourceId);
        extractFeature(*featureJson, _out.features.back(), _proj);
    }

}

}
