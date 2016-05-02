#include "geoJson.h"

#include "data/propertyItem.h"
#include "glm/glm.hpp"
#include "log.h"

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

bool GeoJson::getLine(const JsonValue& _in, const Transform& _proj, Geometry<Point>& _geom) {

    int numPoints = 0;
    Point prev;
    for (auto& point : _in) {
        auto p = getPoint(point, _proj);
        if (numPoints == 0 || p != prev) {
            _geom.addPoint(p);
            numPoints++;
        }
        prev = p;
    }
    if (numPoints > 0) {
        _geom.endLine();
        return true;
    }
    return false;
}

bool GeoJson::getPolygon(const JsonValue& _in, const Transform& _proj, Geometry<Point>& _geom) {

    int numRings = 0;
    for (auto& ring : _in) {
        if (getLine(ring, _proj, _geom)) {
            numRings++;
        }
    }
    if (numRings > 0) {
        _geom.endPoly();
        return true;
    }
    return false;
}

Properties GeoJson::getProperties(const JsonValue& _in, int32_t _sourceId) {

    std::vector<PropertyItem> items;
    items.reserve(_in.MemberCount());

    for (auto& prop : _in.Members()) {

        const auto& name = prop.name.GetString();
        const auto& value = prop.value;
        if (value.IsNumber()) {
            items.emplace_back(name, value.GetDouble());
        } else if (prop.value.IsString()) {
            items.emplace_back(name, value.GetString());
        }
    }

    Properties properties;
    properties.sourceId = _sourceId;
    properties.setSorted(std::move(items));
    properties.sort();

    return properties;
}

void GeoJson::processFeature(const JsonValue& _in, const Transform& _proj, int32_t _sourceId,
                             Feature& _feature, TileDataSink& _sink) {

    _feature.geometry.clear();

    // Copy properties into tile data
    auto properties = _in.FindMember("properties");
    if (properties != _in.MemberEnd()) {
        _feature.props = getProperties(properties->value, _sourceId);
    } else {
        LOGW("Missing 'properties'");
        return;
    }

    // Copy geometry into tile data
    const JsonValue& geometry = _in["geometry"];
    const JsonValue& coords = geometry["coordinates"];
    const std::string& type = geometry["type"].GetString();

    if (type == "Point" || type == "MultiPoint") {
        _feature.geometry.type = GeometryType::points;
    } else if (type == "LineString" || type == "MultiLineString") {
        _feature.geometry.type = GeometryType::lines;
    } else if (type == "Polygon" || type == "MultiPolygon") {
        _feature.geometry.type = GeometryType::polygons;
    } else {
        // TODO: Unhandled GeometryCollection
        LOG("Unhandled type: %s", type.c_str());
        return;
    }

    if (!_sink.matchFeature(_feature)) {
        // LOG("skip feature %d - props %s",
        //     _feature.geometry.type,
        //     _feature.props.toJson().c_str());
        return;
    }

    bool multi = type.compare(0, 5, "Multi") == 0;

    if (_feature.geometry.type == GeometryType::points) {
        if (!multi) {
            _feature.geometry.addPoint(getPoint(coords, _proj));
        } else {
            for (auto& pointCoords : coords) {
                _feature.geometry.addPoint(getPoint(pointCoords, _proj));
            }
        }
    } else if (_feature.geometry.type == GeometryType::lines) {
        if (!multi) {
            getLine(coords, _proj, _feature.geometry);
        } else {
            for (auto& lineCoords : coords) {
                getLine(lineCoords, _proj, _feature.geometry);
            }
        }
    } else if (_feature.geometry.type == GeometryType::polygons) {
        if (!multi) {
            getPolygon(coords, _proj, _feature.geometry);
        } else {
            for (auto& polyCoords : coords) {
                getPolygon(polyCoords, _proj, _feature.geometry);
            }
        }
    }

    _sink.addFeature(_feature);
}

bool GeoJson::processLayer(const JsonValue& _in, const Transform& _proj,
                            int32_t _sourceId, TileDataSink& _sink) {

    auto features = _in.FindMember("features");

    if (features == _in.MemberEnd()) {
        LOGE("GeoJSON missing 'features' member");
        return false;
    }

    Feature feature;

    for (auto& featureValue : features->value) {
        processFeature(featureValue, _proj, _sourceId, feature, _sink);
    }

    return true;

}
}
