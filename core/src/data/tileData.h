#pragma once

#include <vector>
#include <unordered_map>

/* Notes on TileData implementation:

Tile Coordinates:

  A point in the geometry of a tile can be represented with adequate precision using short integers for each dimension.
  (Truthfully, if a tile is meant to represent 256 x 256 pixels on a screen we could use a char, but this can be a future 
  optimization.) The 2D area of a tile could be intuitively parameterized by assigning coordinates relative to one corner
  and assuming positive values along each axis, however the way that our tile data is currently served won't allow this 
  very easily. The geometry we receive from the server does not always fit precisely within the bounds of the tile area, 
  so our geometry coordinates must be able to address locations outside of the tile area to some degree. For practical
  purposes, we can assume that geometry will not be more than 1 tile size unit away from the edge of its containing tile. 
  Given this constraint, it is actually a better use of our range of numerical precision to use the centers of tiles as 
  the origin of our coordinates. 

  Our tile-local coordinate system is illustrated below:

  (-8192, 8192) -------------------- (8192, 8192)
               |                    |
               |      +y ^          |
               |         | (0, 0)   |
               |       ----- > +x   |
               |         |          |
               |                    |
               |                    |
  (-8192, -8192)-------------------- (8192, -8192)

  Using signed short integers for local coordinates, this allows plenty of precision within a tile while also accurately
  representing points that falls outside the tile boundaries by up to 1.5 tile-lengths (coordinate range -32768 to 32767).

Data heirarchy:

  TileData is a heirarchical container of structs. 

    TileData
    {
        Layer1
        {
            Feature1
            Feature2
            ...
        }
        Layer2
        ...
    }

  At the top level, a TileData contains a list of Layer structs. 

  Layer
  {
    String name;
    Features...
  }

  A Layer contains a string containing its name and a list of Feature structs.

  Feature
  {
    PList props;
    Geometry geometry;
  }

*/

enum GeometryType {
    POINTS,
    LINES,
    POLYGONS
}

struct Point {
    
    Point (signed short _x = 0, signed short _y = 0): x(_x), y(_y) {}
    
    signed short x;
    signed short y;
};

typedef std::vector<Point> Line;

typedef std::vector<Line> Polygon;

struct Properties {
    
    std::unordered_map<std::string, std::string> stringProps;
    std::unordered_map<std::string, float> numericalProps;
    
};

struct Feature {
    
    GeometryType geometryType;
    
    std::vector<Point> points;
    std::vector<Line> lines;
    std::vector<Polygon> polygons;
    
    Properties props;
    
};

struct Layer {
    
    std::string name;
    
    std::vector<Feature> features;
    
};

struct TileData {
    
    const static signed short extent = 8192;
    
    std::vector<Layer> layers;
    
};

