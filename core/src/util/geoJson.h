#pragma once

#include "data/tileData.h"
#include "util/json.h"

#include <functional>

namespace Tangram {

namespace GeoJson {

using Transform = std::function<Point(glm::dvec2 _lonLat)>;

Point getPoint(const JsonValue& _in, const Transform& _proj);

Line getLine(const JsonValue& _in, const Transform& _proj);

Polygon getPolygon(const JsonValue& _in, const Transform& _proj);

Feature getFeature(const JsonValue& _in, const Transform& _proj, int32_t _sourceId);

Layer getLayer(const JsonDocument::MemberIterator& _in, const Transform& _proj, int32_t _sourceId);

}

}
