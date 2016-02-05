#pragma once

#include "glm/vec3.hpp"
#include "data/properties.h"

#include <vector>
#include <string>

/*

Tile Coordinates:

  A point in the geometry of a tile is represented with 32-bit floating point
  x, y, and z coordinates. Coordinates represent normalized displacement from
  the origin (i.e. lower-left corner) of a tile.

  (0.0, 1.0) ---------- (1.0, 1.0)
            |          |             N
            ^ +y       |          W <|> E
            |          |             S
            |    +x    |
  (0.0, 0.0) ----->---- (1.0, 0.0)

  Coordinates that fall outside the range [0.0, 1.0] are permissible, as tile
  servers may choose not to clip certain geometries to tile boundaries, but these
  points are clipped in the client-side geometry processing.

  Z coordinates are expected to be normalized to the same scale as x, y coordinates.

Data heirarchy:

  TileData is a heirarchical container of structs modeled after the geoJSON spec:
  http://geojson.org/geojson-spec.html

  A <TileData> contains a collection of <Layer>s

  A <Layer> contains a name and a collection of <Feature>s

  A <Feature> contains a <GeometryType> denoting what variety of geometry is
  contained in the feature, a <Properties> struct describing the feature, and
  one collection each of <Point>s, <Line>s, and <Polygon>s. Only the geometry
  collection corresponding to the feature's geometryType should contain data.

  A <Properties> contains a sorted vector of key-value pairs storing the
  properties of a <Feature>

  A <Polygon> is a collection of <Line>s representing the contours of a polygon.
  Contour winding rules follow the conventions of the OpenGL red book described
  here: http://www.glprogramming.com/red/chapter11.html

  A <Line> is a collection of <Point>s.

  A <Point> is 3 32-bit floating point coordinates representing x, y, and z
  (in that order).

*/
namespace Tangram {

enum GeometryType {
    unknown,
    points,
    lines,
    polygons
};

typedef glm::vec3 Point;

typedef std::vector<Point> Line;

typedef std::vector<Line> Polygon;

struct Feature {
    Feature() {}
    Feature(int32_t _sourceId) { props.sourceId = _sourceId; }

    GeometryType geometryType = GeometryType::polygons;

    std::vector<Point> points;
    std::vector<Line> lines;
    std::vector<Polygon> polygons;

    Properties props;
};

struct Layer {

    Layer(const std::string& _name) : name(_name) {}

    std::string name;

    std::vector<Feature> features;

};

struct TileData {

    std::vector<Layer> layers;

};

}
