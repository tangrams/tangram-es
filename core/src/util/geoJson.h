#pragma once

#include "data/tileData.h"
#include "util/json.h"

#include <functional>

namespace Tangram {

namespace GeoJson {

using Transform = std::function<Point(glm::dvec2 _lonLat)>;

bool isFeatureCollection(const JsonValue& _in);

Point getPoint(const JsonValue& _in, const Transform& _proj);

bool getLine(const JsonValue& _in, const Transform& _proj, Geometry<Point>& _geom);

bool getPolygon(const JsonValue& _in, const Transform& _proj, Geometry<Point>& _geom);

Properties getProperties(const JsonValue& _in, int32_t _sourceId);

void processFeature(const JsonValue& _in, const Transform& _proj, int32_t _sourceId,
                    Feature& _feature, TileDataSink& _sink);

bool processLayer(const JsonValue& _in, const Transform& _proj,
                  int32_t _sourceId, TileDataSink& _sink);


}

}
