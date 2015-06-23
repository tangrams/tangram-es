#pragma once

#include "flyweight/object.hpp"
#include "core/variant.hpp"

#include <vector>
#include <string>
#include <unordered_map>

#include "glm/vec3.hpp"

/* Notes on TileData implementation:

Tile Coordinates:

  A point in the geometry of a tile is represented with 32-bit floating point x, y, and z coordinates. Coordinates represent
  normalized displacement from the origin (i.e. center) of a tile.

  (-1.0, 1.0) -------------------- (1.0, 1.0)
             |                    |
             |      +y ^          |
             |         | (0, 0)   |
             |       ----- > +x   |
             |         |          |
             |                    |
             |                    |
  (-1.0, -1.0)-------------------- (1.0, -1.0)

  Coordinates that fall outside the range [-1.0, 1.0] are permissible, as tile servers may chose not to clip certain geometries
  to tile boundaries, but in the future these points may be clipped in the client-side geometry processing. 
 
  Z coordinates are expected to be normalized to the same scale as x asnd y coordinates.

Data heirarchy:

  TileData is a heirarchical container of structs modeled after the geoJSON spec: http://geojson.org/geojson-spec.html

  A <TileData> contains a collection of <Layer>s
 
  A <Layer> contains a name and a collection of <Feature>s
 
  A <Feature> contains a <GeometryType> denoting what variety of geometry is contained in the feature, a <Properties> struct
  describing the feature, and one collection each of <Point>s, <Line>s, and <Polygon>s. Only the geometry collection corresponding
  to the feature's geometryType should contain data. 
 
  A <Properties> contains two key-value maps, one for string properties and one for numeric (floating point) properties. 
 
  A <Polygon> is a collection of <Line>s representing the contours of a polygon. Contour winding rules follow the conventions of
  the OpenGL red book described here: http://www.glprogramming.com/red/chapter11.html
 
  A <Line> is a collection of <Point>s.
 
  A <Point> is 3 32-bit floating point coordinates representing x, y, and z (in that order).

*/

enum GeometryType {
    UNKNOWN,
    POINTS,
    LINES,
    POLYGONS
};

typedef glm::vec3 Point;

typedef std::vector<Point> Line;

typedef std::vector<Line> Polygon;

using TagKey = flyweight::object<std::string>;
using Value = core::variant<std::string, float>;

static TagKey TAG_KEY_NAME { "name" };
static TagKey TAG_KEY_HEIGHT { "height" };
static TagKey TAG_KEY_MIN_HEIGHT { "min_height" };
static TagKey TAG_KEY_ZOOM { "zoom" };
static TagKey TAG_KEY_SORT_KEY { "sort_key" };

namespace std
{
template <> struct hash<TagKey>
{
  std::size_t operator()(const TagKey& str) const {
    return hash<std::string>()(str);
    }
};
}

typedef std::unordered_map<TagKey, Value> Properties;

class Props {
 public:
  static const std::string& GetString(const Properties& _props, const TagKey& _key, const std::string& _fallback) {
    auto it = _props.find(_key);
    if(it == _props.end())
      return _fallback;
    
    auto* result = core::get<0>(&it->second);
    if (!result)
      return _fallback;

    return *result;
  };

  static float GetFloat(const Properties& _props, const TagKey& _key, float _fallback) {
    auto it = _props.find(_key);
    if(it == _props.end())
      return _fallback;
    
    auto* result = core::get<1>(&it->second);

    if (!result)
      return _fallback;

    return *result;
  };
};

struct Feature {
    
    GeometryType geometryType = GeometryType::POLYGONS;
    
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

