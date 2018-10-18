#pragma once

#include "tile/tileID.h"
#include "util/geom.h"
#include "util/types.h"

namespace Tangram {

/*
Projected Meters
----------------
 Map projections define a 2D coordinate system whose origin is at longitude and
 latitude zero and whose minimum and maximum values are given by ProjectedBounds().

 +------------------+
 |        |         |
 |     +y ^         |
 |        |   +x    |     N
 |---<----+---->----|  W <|> E
 |        |(0,0)    |     S
 |        v         |
 |        |         |
 +------------------+
*/
using ProjectedMeters = glm::dvec2;

/*
Tile Coordinates
----------------
 Tiles are addressed within a 2D coordinate system at each zoom level whose
 origin is at the upper-left corner of the spherical mercator projection space.
 The space is divided into 2^z tiles at each zoom, so that the boundary of the
 coordinates in each dimension is 2^z.

 +------>-----------+
 |(0,0) +x|         | (2^z,0)
 |        |         |
 v +y     |         |     N
 |--------+---------|  W <|> E
 |        |         |     S
 |        |         |
 |        |         |
 +------------------+
  (0,2^z)             (2^z,2^z)
*/
struct TileCoordinates {
    double x;
    double y;
    int z;
};

/**
 * Spherical Mercator projection.
 */
class MapProjection {

public:

    constexpr static double EARTH_RADIUS_METERS = 6378137.0;
    constexpr static double EARTH_HALF_CIRCUMFERENCE_METERS = PI * EARTH_RADIUS_METERS;
    constexpr static double EARTH_CIRCUMFERENCE_METERS = 2 * PI * EARTH_RADIUS_METERS;
    constexpr static double MAX_LATITUDE_DEGREES = 85.05112878;

    MapProjection() = delete;

    static ProjectedMeters lngLatToProjectedMeters(LngLat lngLat);

    static LngLat projectedMetersToLngLat(ProjectedMeters meters);

    static ProjectedMeters tileCoordinatesToProjectedMeters(TileCoordinates tileCoordinates);

    static ProjectedMeters tileSouthWestCorner(TileID tile);

    static ProjectedMeters tileCenter(TileID tile);

    static BoundingBox tileBounds(TileID tile);

    static double metersPerTileAtZoom(int zoom);

    // Bounds of the map projection in projected meters.
    static BoundingBox mapProjectedMetersBounds();

    // Bounds of the map projection in longitude and latitude.
    static BoundingBox mapLngLatBounds();

    static constexpr double tileSize() { return 256; }

};

} // namespace Tangram
