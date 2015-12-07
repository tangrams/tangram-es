#pragma once

#include "rapidjson/document.h"
#include "data/tileData.h"

#include <vector>
#include <functional>

namespace Tangram {

namespace GeoJson {

typedef std::function<Point(glm::dvec2 _lonLat)> tileProjectionFn;

void extractPoint(const rapidjson::Value& _in, Point& _out, const tileProjectionFn& _proj);

void extractLine(const rapidjson::Value& _in, Line& _out, const tileProjectionFn& _proj);

void extractPoly(const rapidjson::Value& _in, Polygon& _out, const tileProjectionFn& _proj);

void extractFeature(const rapidjson::Value& _in, Feature& _out, const tileProjectionFn& _proj);

void extractLayer(int32_t _sourceId, const rapidjson::Value& _in, Layer& _out, const tileProjectionFn& _proj);

}

}
