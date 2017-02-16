#pragma once

#include "data/tileData.h"
#include "util/json.h"

#include "glm/vec2.hpp"
#include <functional>

namespace Tangram {

namespace TopoJson {

using Transform = std::function<Point(glm::dvec2 _lonLat)>;

struct Topology {
    glm::dvec2 scale = { 1., 1. };
    glm::dvec2 translate = { 0., 0. };
    std::vector<Line> arcs;
    Transform proj;
};

Topology getTopology(const JsonDocument& _document, const Transform& _proj);

Point getPoint(const JsonValue& _coordinates, const Topology& _topology, glm::ivec2& _cursor);

Line getLine(const JsonValue& _arcs, const Topology& _topology);

Polygon getPolygon(const JsonValue& _arcs, const Topology& _topology);

Feature getFeature(const JsonValue& _geometry, const Topology& _topology, int32_t _sourceId);

Layer getLayer(JsonValue::MemberIterator& _object, const Topology& _topology, int32_t _sourceId);

}

}
