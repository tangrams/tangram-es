#include "util/geoJson.h"

#include "data/propertyItem.h"
#include "log.h"

#include "glm/glm.hpp"

namespace Tangram {

bool GeoJson::isFeatureCollection(const JsonValue& _in) {

    // A FeatureCollection must have a "type" of "FeatureCollection"
    // and a member named "features" that is an array.
    // http://geojson.org/geojson-spec.html#feature-collection-objects

    auto type = _in.FindMember("type");
    if (type == _in.MemberEnd() || !type->value.IsString() ||
        std::strcmp(type->value.GetString(), "FeatureCollection") != 0) {
        return false;
    }

    auto features = _in.FindMember("features");
    if (features == _in.MemberEnd() || !features->value.IsArray()) {
        return false;
    }

    return true;

}

Point GeoJson::getPoint(const JsonValue& _in, const Transform& _proj) {
    return _proj(glm::dvec2(_in[0].GetDouble(), _in[1].GetDouble()));
}

Line GeoJson::getLine(const JsonValue& _in, const Transform& _proj) {

    Line line;
    for (auto itr = _in.Begin(); itr != _in.End(); ++itr) {
        line.push_back(getPoint(*itr, _proj));
    }
    return line;

}

Polygon GeoJson::getPolygon(const JsonValue& _in, const Transform& _proj) {

    Polygon polygon;
    for (auto itr = _in.Begin(); itr != _in.End(); ++itr) {
        polygon.push_back(getLine(*itr, _proj));
    }
    return polygon;

}

Properties GeoJson::getProperties(const JsonValue& _in, int32_t _sourceId) {

    std::vector<PropertyItem> items;
    items.reserve(_in.MemberCount());

    for (auto it = _in.MemberBegin(); it != _in.MemberEnd(); ++it) {

        const auto& name = it->name.GetString();
        const auto& value = it->value;
        if (value.IsNumber()) {
            items.emplace_back(name, value.GetDouble());
        } else if (it->value.IsString()) {
            items.emplace_back(name, value.GetString());
        } else if (it->value.IsBool()) {
            items.emplace_back(name, double(value.GetBool()));
        }
    }

    Properties properties;
    properties.sourceId = _sourceId;
    properties.setSorted(std::move(items));
    properties.sort();

    return properties;

}

Feature GeoJson::getFeature(const JsonValue& _in, const Transform& _proj, int32_t _sourceId) {

    Feature feature;

    // Copy properties into tile data
    auto properties = _in.FindMember("properties");
    if (properties != _in.MemberEnd()) {
        feature.props = getProperties(properties->value, _sourceId);
    }

    // Copy geometry into tile data
    const JsonValue& geometry = _in["geometry"];
    const JsonValue& coords = geometry["coordinates"];
    const std::string& geometryType = geometry["type"].GetString();

    if (geometryType.compare("Point") == 0) {

        feature.geometryType = GeometryType::points;
        feature.points.push_back(getPoint(coords, _proj));

    } else if (geometryType.compare("MultiPoint") == 0) {

        feature.geometryType = GeometryType::points;
        for (auto pointCoords = coords.Begin(); pointCoords != coords.End(); ++pointCoords) {
            feature.points.push_back(getPoint(*pointCoords, _proj));
        }

    } else if (geometryType.compare("LineString") == 0) {

        feature.geometryType = GeometryType::lines;
        feature.lines.push_back(getLine(coords, _proj));

    } else if (geometryType.compare("MultiLineString") == 0) {

        feature.geometryType = GeometryType::lines;
        for (auto lineCoords = coords.Begin(); lineCoords != coords.End(); ++lineCoords) {
            feature.lines.push_back(getLine(*lineCoords, _proj));
        }

    } else if (geometryType.compare("Polygon") == 0) {

        feature.geometryType = GeometryType::polygons;
        feature.polygons.push_back(getPolygon(coords, _proj));

    } else if (geometryType.compare("MultiPolygon") == 0) {

        feature.geometryType = GeometryType::polygons;
        for (auto polyCoords = coords.Begin(); polyCoords != coords.End(); ++polyCoords) {
            feature.polygons.push_back(getPolygon(*polyCoords, _proj));
        }

    }

    return feature;

}

Layer GeoJson::getLayer(const JsonValue& _in, const Transform& _proj, int32_t _sourceId) {

    Layer layer("");

    auto features = _in.FindMember("features");

    if (features == _in.MemberEnd()) {
        LOGE("GeoJSON missing 'features' member");
        return layer;
    }

    for (auto featureIt = features->value.Begin(); featureIt != features->value.End(); ++featureIt) {
        layer.features.push_back(getFeature(*featureIt, _proj, _sourceId));
    }

    return layer;

}

}
