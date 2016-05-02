#pragma once

#include "data/properties.h"
#include "data/geometry.h"

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


struct Feature {
    Feature() {}
    Feature(int32_t _sourceId) { props.sourceId = _sourceId; }

    Properties props;
    Geometry<Point> geometry;

    auto& points() const { return geometry.points(); }
    auto lines() const { return geometry.lines(); }
    auto polygons() const { return geometry.polygons(); }
};

struct TileDataSink {
    virtual bool beginLayer(const std::string& _layer) = 0;
    virtual bool matchFeature(const Feature& _feature) = 0;
    virtual void addFeature(const Feature& _feature) = 0;
};

/*** Unused - but may be handy for writing tests. ***/
struct Layer {
    Layer(const std::string& _name) : name(_name) {}

    std::string name;

    std::vector<Feature> features;

    void process(TileDataSink& _sink) {
        for (auto& feat : features) {
            if (_sink.matchFeature(feat)) {
                _sink.addFeature(feat);
            }
        }
    }
};

struct TileData {

    std::vector<Layer> layers;

    void process(TileDataSink& _sink) {
        for (auto& layer : layers) {
            if (_sink.beginLayer(layer.name)) {
                layer.process(_sink);
            }
        }
    }
};

}
