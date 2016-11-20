#include "topoJson.h"
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
    for (auto& jsonArc : jsonArcs) {

        if (!jsonArc.IsArray()) { // According to spec, jsonArc.Size() >= 2 should also hold
            continue;
        }

        std::vector<Point> arc;
        arc.reserve(jsonArc.Size());

        // Quantized position
        glm::ivec2 q;

        for (auto& jsonCoords : jsonArc) {

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

bool getLine(const JsonValue& _arcs, const Topology& _topology, Geometry<Point>& _geom) {

    if (!_arcs.IsArray()) { return false; }

    int numPoints = 0;

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
            _geom.addPoint(*pointIt);
            numPoints++;
        }
    }

    if (numPoints > 0) {
        _geom.endLine();
        return true;
    }
    return false;
}

bool getPolygon(const JsonValue& _arcSets, const Topology& _topology, Geometry<Point>& _geom) {

    if (!_arcSets.IsArray()) { return false; }

    int numRings = 0;
    for (auto& arcSet : _arcSets) {
        if (getLine(arcSet, _topology, _geom)) {
            numRings++;
        }
    }
    if (numRings > 0) {
        _geom.endPoly();
        return true;
    }

    return false;
}

void processFeature(const JsonValue& _geometry, const Topology& _topology, int32_t _source,
                    Feature& _feature, TileDataSink& _sink) {

    static const JsonValue keyProperties("properties");
    static const JsonValue keyType("type");
    static const JsonValue keyCoordinates("coordinates");
    static const JsonValue keyArcs("arcs");

    _feature.geometry.clear();

    auto propertiesIt = _geometry.FindMember(keyProperties);
    if (propertiesIt != _geometry.MemberEnd() && propertiesIt->value.IsObject()) {
        _feature.props = GeoJson::getProperties(propertiesIt->value, _source);
    }

    std::string type;
    auto typeIt = _geometry.FindMember(keyType);
    if (typeIt != _geometry.MemberEnd() && typeIt->value.IsString()) {
        type = typeIt->value.GetString();
    }

    if (type == "Point" || type == "MultiPoint") {
        _feature.geometry.type = GeometryType::points;
    } else if (type == "LineString" || type == "MultiLineString") {
        _feature.geometry.type = GeometryType::lines;
    } else if (type == "Polygon" || type == "MultiPolygon") {
        _feature.geometry.type = GeometryType::polygons;
    } else {
        // TODO: Unhandled GeometryCollection
        return;
    }

    if (!_sink.matchFeature(_feature)) {
        return;
    }

    bool multi = type.compare(0, 5, "Multi") == 0;

    if (_feature.geometry.type == GeometryType::points) {
        auto coordinatesIt = _geometry.FindMember(keyCoordinates);
        if (coordinatesIt != _geometry.MemberEnd()) {
            if (!multi) {
                glm::ivec2 cursor;
                _feature.geometry.addPoint(getPoint(coordinatesIt->value,
                                                    _topology, cursor));

            } else if (coordinatesIt->value.IsArray()) {
                for (auto& point : coordinatesIt->value) {
                    glm::ivec2 cursor;
                    _feature.geometry.addPoint(getPoint(point, _topology, cursor));
                }
            }
        }
    } else if (_feature.geometry.type == GeometryType::lines) {
        auto arcsIt = _geometry.FindMember(keyArcs);
        if (arcsIt != _geometry.MemberEnd()) {
            if (!multi) {
                getLine(arcsIt->value, _topology, _feature.geometry);

            } else if (arcsIt->value.IsArray()) {
                for (auto& arcList : arcsIt->value) {
                    getLine(arcList, _topology, _feature.geometry);
                }
            }
        }
    } else if (_feature.geometry.type == GeometryType::polygons) {
        auto arcsIt = _geometry.FindMember(keyArcs);
        if (arcsIt != _geometry.MemberEnd()) {
            if (!multi) {
                getPolygon(arcsIt->value, _topology, _feature.geometry);

            } else if (arcsIt->value.IsArray()) {
                for (auto& arcList : arcsIt->value) {
                    getPolygon(arcList, _topology, _feature.geometry);
                }
            }
        }
    }
    _sink.addFeature(_feature);
}

bool processLayer(JsonValue::MemberIterator& _objectIt, const Topology& _topology,
                  int32_t _source, TileDataSink& _sink) {

    if (!_sink.beginLayer(_objectIt->name.GetString())) {
        return false;
    }

    Feature feature;

    JsonValue& object = _objectIt->value;
    auto type = object.FindMember("type");
    if (type != object.MemberEnd() &&
        strcmp("GeometryCollection", type->value.GetString()) == 0) {

        auto geometries = object.FindMember("geometries");

        if (geometries != object.MemberEnd() && geometries->value.IsArray()) {
            for (auto& it : geometries->value) {
                processFeature(it, _topology, _source, feature, _sink);
            }
        }
    }
    return true;
}

}
}
