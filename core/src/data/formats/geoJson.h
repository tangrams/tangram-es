#pragma once

#include "data/tileData.h"
#include "util/json.h"
#include "util/types.h"
#include <functional>
#include <memory>

namespace Tangram {

class TileTask;

namespace GeoJson {

using Transform = std::function<Point(LngLat _lngLat)>;

bool isFeatureCollection(const JsonValue& _in);

Point getPoint(const JsonValue& _in, const Transform& _proj);

Line getLine(const JsonValue& _in, const Transform& _proj);

Polygon getPolygon(const JsonValue& _in, const Transform& _proj);

Properties getProperties(const JsonValue& _in, int32_t _sourceId);

Feature getFeature(const JsonValue& _in, const Transform& _proj, int32_t _sourceId);

Layer getLayer(const JsonValue& _in, const Transform& _proj, int32_t _sourceId);

std::shared_ptr<TileData> parseTile(const TileTask& _task, int32_t _sourceId);

} // namespace GeoJson

} // namespace Tangram
