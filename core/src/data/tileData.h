#pragma once

#include "glm/vec3.hpp"
#include "util/variant.h"

#include <vector>
#include <string>
#include <unordered_map>

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

int polygonSize(const Polygon& _polygon);

struct Properties {

    struct Item {
        Item(std::string _key, Value _value) :
            key(std::move(_key)), value(std::move(_value)) {}

        std::string key;
        Value value;
        bool operator<(const Item& _rhs) const { return key < _rhs.key; }
    };

    Properties() {}
    Properties(const Properties& _other) = default;
    Properties(std::vector<Item>&& _items);
    Properties& operator=(Properties&& _other);

    const Value& get(const std::string& key) const;

    void sort();

    void clear() { props.clear(); }

    bool contains(const std::string& key) const {
        return !get(key).is<none_type>();
    }

    bool getNumeric(const std::string& key, double& value) const {
        auto& it = get(key);
        if (it.is<float>()) {
            value = it.get<float>();
            return true;
        } else if (it.is<int64_t>()) {
            value = it.get<int64_t>();
            return true;
        }
        return false;
    }

    double getNumeric(const std::string& key) const {
        auto& it = get(key);
        if (it.is<float>()) {
            return it.get<float>();
        } else if (it.is<int64_t>()) {
            return it.get<int64_t>();
        }
        return 0;
    }
    bool getString(const std::string& key, std::string& value) const {
        auto& it = get(key);
        if (it.is<std::string>()) {
            value = it.get<std::string>();
            return true;
        }
        return false;
    }

    const std::string& getString(const std::string& key) const;

    std::string getAsString(const std::string& key) const;

    template <typename... Args> void add(std::string key, Args&&... args) {
        props.emplace_back(std::move(key), Value{std::forward<Args>(args)...});
        sort();
    }

    const auto& items() const { return props; }

private:
    std::vector<Item> props;
};

struct Feature {

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
