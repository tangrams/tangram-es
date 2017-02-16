#include "util/topoJson.h"

#include "data/propertyItem.h"
#include "util/geoJson.h"

namespace Tangram {
namespace TopoJson {

Topology getTopology(const JsonDocument& _document, const Transform& _proj) {

    Topology topo;

    topo.proj = _proj;

    auto transform = _document.FindMember("transform");
    if (transform != _document.MemberEnd()) {
        auto scale = transform->value.FindMember("scale");
        if (scale != transform->value.MemberEnd() && scale->value.Size() == 2) {
            topo.scale = { scale->value[0].GetDouble(), scale->value[1].GetDouble() };
        }
        auto translate = transform->value.FindMember("translate");
        if (translate != transform->value.MemberEnd() && translate->value.Size() == 2) {
            topo.translate = { translate->value[0].GetDouble(), translate->value[1].GetDouble() };
        }
    }

    // Quantized, delta-encoded 'arcs' in Json
    auto jsonArcList = _document.FindMember("arcs");

    if (jsonArcList == _document.MemberEnd()) {
        return topo;
    }

    const auto& jsonArcs = jsonArcList->value;

    if (!jsonArcs.IsArray()) {
        return topo;
    }

    topo.arcs.reserve(jsonArcs.Size());

    // Decode and transform the points that make up 'arcs'
    for (auto jsonArcsIt = jsonArcs.Begin(); jsonArcsIt != jsonArcs.End(); ++jsonArcsIt) {

        const auto& jsonArc = *jsonArcsIt;

        if (!jsonArc.IsArray()) { // According to spec, jsonArc.Size() >= 2 should also hold
            continue;
        }

        Line arc;
        arc.reserve(jsonArc.Size());

        // Quantized position
        glm::ivec2 q;

        for (auto jsonCoordsIt = jsonArc.Begin(); jsonCoordsIt != jsonArc.End(); ++jsonCoordsIt) {

            const auto& jsonCoords = *jsonCoordsIt;

            arc.push_back(getPoint(jsonCoords, topo, q));
        }

        topo.arcs.push_back(arc);
    }

    return topo;
}

Point getPoint(const JsonValue& _coordinates, const Topology& _topology, glm::ivec2& _cursor) {

    if (!_coordinates.IsArray() || _coordinates.Size() < 2) {
        return Point();
    }

    _cursor.x += _coordinates[0].GetInt();
    _cursor.y += _coordinates[1].GetInt();

    return _topology.proj(glm::dvec2(_cursor) * _topology.scale + _topology.translate);

}

Line getLine(const JsonValue& _arcs, const Topology& _topology) {

    Line line;

    if (!_arcs.IsArray()) {
        return line;
    }

    for (auto arcIt = _arcs.Begin(); arcIt != _arcs.End(); ++arcIt) {

        auto index = arcIt->GetInt();
        bool reverse = false;
        if (index < 0) {
            reverse = true;
            index = -1 - index;
        }

        if (index < 0 || (std::vector<Line>::size_type)index >= _topology.arcs.size()) {
            continue;
        }

        const auto& arc = _topology.arcs[index];

        auto begin = arc.begin();
        auto end = arc.end();
        size_t inc = 1;
        if (reverse) {
            begin = arc.end() - 1;
            end = arc.begin() - 1;
            inc = -inc;
        }

        // If a line is made from multiple arcs, the first position of an arc must
        // be equal to the last position of the previous arc. So when reconstructing
        // the geometry, the first position of each arc except the first may be dropped
        if (arcIt != _arcs.Begin()) {
            begin = begin + inc;
        }

        for (auto pointIt = begin; pointIt != end; pointIt += inc) {
            line.push_back(*pointIt);
        }

    }

    return line;

}

Polygon getPolygon(const JsonValue& _arcSets, const Topology& _topology) {

    Polygon polygon;

    if (!_arcSets.IsArray()) {
        return polygon;
    }

    for (auto arcSetIt = _arcSets.Begin(); arcSetIt != _arcSets.End(); ++arcSetIt) {

        auto ring = getLine(*arcSetIt, _topology);
        polygon.push_back(ring);

    }

    return polygon;

}

Feature getFeature(const JsonValue& _geometry, const Topology& _topology, int32_t _source) {

    static const JsonValue keyProperties("properties");
    static const JsonValue keyType("type");
    static const JsonValue keyCoordinates("coordinates");
    static const JsonValue keyArcs("arcs");

    Feature feature;

    auto propertiesIt = _geometry.FindMember(keyProperties);
    if (propertiesIt != _geometry.MemberEnd() && propertiesIt->value.IsObject()) {
        feature.props = GeoJson::getProperties(propertiesIt->value, _source);
    }

    std::string type;
    auto typeIt = _geometry.FindMember(keyType);
    if (typeIt != _geometry.MemberEnd() && typeIt->value.IsString()) {
        type = typeIt->value.GetString();
    }

    if (type == "Point") {
        feature.geometryType = GeometryType::points;
        auto coordinatesIt = _geometry.FindMember(keyCoordinates);
        if (coordinatesIt != _geometry.MemberEnd()) {
            glm::ivec2 cursor;
            feature.points.push_back(getPoint(coordinatesIt->value, _topology, cursor));
        }
    } else if (type == "MultiPoint") {
        feature.geometryType = GeometryType::points;
        auto coordinatesIt = _geometry.FindMember(keyCoordinates);
        if (coordinatesIt != _geometry.MemberEnd() && coordinatesIt->value.IsArray()) {
            auto& coordinates = coordinatesIt->value;
            for (auto point = coordinates.Begin(); point != coordinates.End(); ++point) {
                glm::ivec2 cursor;
                feature.points.push_back(getPoint(*point, _topology, cursor));
            }
        }
    } else if (type == "LineString") {
        feature.geometryType = GeometryType::lines;
        auto arcsIt = _geometry.FindMember(keyArcs);
        if (arcsIt != _geometry.MemberEnd()) {
            feature.lines.push_back(getLine(arcsIt->value, _topology));
        }
    } else if (type == "MultiLineString") {
        feature.geometryType = GeometryType::lines;
        auto arcsIt = _geometry.FindMember(keyArcs);
        if (arcsIt != _geometry.MemberEnd() && arcsIt->value.IsArray()) {
            auto& arcs = arcsIt->value;
            for (auto arcList = arcs.Begin(); arcList != arcs.End(); ++arcList) {
                feature.lines.push_back(getLine(*arcList, _topology));
            }
        }
    } else if (type == "Polygon") {
        feature.geometryType = GeometryType::polygons;
        auto arcsIt = _geometry.FindMember(keyArcs);
        if (arcsIt != _geometry.MemberEnd()) {
            feature.polygons.push_back(getPolygon(arcsIt->value, _topology));
        }
    } else if (type == "MultiPolygon") {
        feature.geometryType = GeometryType::polygons;
        auto arcsIt = _geometry.FindMember(keyArcs);
        if (arcsIt != _geometry.MemberEnd() && arcsIt->value.IsArray()) {
            auto& arcs = arcsIt->value;
            for (auto arcList = arcs.Begin(); arcList != arcs.End(); ++arcList) {
                feature.polygons.push_back(getPolygon(*arcList, _topology));
            }
        }
    } else if (type == "GeometryCollection") {
        // Not handled
    }

    return feature;

}

Layer getLayer(JsonValue::MemberIterator& _objectIt, const Topology& _topology, int32_t _source) {

    Layer layer(_objectIt->name.GetString());

    JsonValue& object = _objectIt->value;
    auto type = object.FindMember("type");
    if (type != object.MemberEnd() && strcmp("GeometryCollection", type->value.GetString()) == 0) {
        auto geometries = object.FindMember("geometries");
        if (geometries != object.MemberEnd() && geometries->value.IsArray()) {
            for (auto it = geometries->value.Begin(); it != geometries->value.End(); ++it) {
                layer.features.push_back(getFeature(*it, _topology, _source));
            }
        }
    }

    return layer;

}

}
}
