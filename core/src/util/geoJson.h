#pragma once

#include "rapidjson/document.h"
#include "data/tileData.h"

#include <vector>

namespace Tangram {

class Tile;

namespace GeoJson {

void extractPoint(const rapidjson::Value& _in, Point& _out, const Tile& _tile);

void extractLine(const rapidjson::Value& _in, Line& _out, const Tile& _tile);

void extractPoly(const rapidjson::Value& _in, Polygon& _out, const Tile& _tile);

void extractFeature(const rapidjson::Value& _in, Feature& _out, const Tile& _tile);

void extractLayer(int32_t _sourceId, const rapidjson::Value& _in, Layer& _out, const Tile& _tile);

}

}
