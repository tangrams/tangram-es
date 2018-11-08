#include "mapProjection.h"

namespace Tangram {

ProjectedMeters MapProjection::lngLatToProjectedMeters(LngLat lngLat) {
    ProjectedMeters meters;
    meters.x = lngLat.longitude * MapProjection::EARTH_HALF_CIRCUMFERENCE_METERS / 180.0;
    meters.y = log(tan(PI * 0.25 + lngLat.latitude * PI / 360.0)) * MapProjection::EARTH_RADIUS_METERS;
    return meters;
}

LngLat MapProjection::projectedMetersToLngLat(ProjectedMeters meters) {
    LngLat lngLat;
    lngLat.longitude = meters.x * 180.0 / MapProjection::EARTH_HALF_CIRCUMFERENCE_METERS;
    lngLat.latitude = (2.0 * atan(exp(meters.y / MapProjection::EARTH_RADIUS_METERS)) - PI * 0.5) * 180 / PI;
    return lngLat;
}

ProjectedMeters MapProjection::tileCoordinatesToProjectedMeters(TileCoordinates tileCoordinates) {
    double metersPerTile = metersPerTileAtZoom(tileCoordinates.z);
    ProjectedMeters projectedMeters;
    projectedMeters.x = tileCoordinates.x * metersPerTile - MapProjection::EARTH_HALF_CIRCUMFERENCE_METERS;
    projectedMeters.y = MapProjection::EARTH_HALF_CIRCUMFERENCE_METERS - tileCoordinates.y * metersPerTile;
    return projectedMeters;
}

ProjectedMeters MapProjection::tileSouthWestCorner(TileID tile) {
    TileCoordinates tileCoordinates{static_cast<double>(tile.x), static_cast<double>(tile.y + 1), tile.z};
    return tileCoordinatesToProjectedMeters(tileCoordinates);
}

ProjectedMeters MapProjection::tileCenter(TileID tile) {
    TileCoordinates tileCoordinates{static_cast<double>(tile.x) + 0.5, static_cast<double>(tile.y) + 0.5, tile.z};
    return tileCoordinatesToProjectedMeters(tileCoordinates);
}

BoundingBox MapProjection::tileBounds(TileID tile) {
    TileCoordinates minTileCoordinates{static_cast<double>(tile.x), static_cast<double>(tile.y) + 1.0, tile.z};
    TileCoordinates maxTileCoordinates{static_cast<double>(tile.x) + 1.0, static_cast<double>(tile.y), tile.z};
    auto minProjectedMeters = tileCoordinatesToProjectedMeters(minTileCoordinates);
    auto maxProjectedMeters = tileCoordinatesToProjectedMeters(maxTileCoordinates);
    return BoundingBox{minProjectedMeters, maxProjectedMeters};
}

double MapProjection::metersPerTileAtZoom(int zoom) {
    return MapProjection::EARTH_CIRCUMFERENCE_METERS / (1 << zoom);
}

BoundingBox MapProjection::mapLngLatBounds() {
    // Reference: https://en.wikipedia.org/wiki/Mercator_projection#Truncation_and_aspect_ratio
    return { glm::dvec2(-180, -MAX_LATITUDE_DEGREES), glm::dvec2(180, MAX_LATITUDE_DEGREES) };
}

BoundingBox MapProjection::mapProjectedMetersBounds() {
    BoundingBox bound = mapLngLatBounds();
    return {lngLatToProjectedMeters({bound.min.x, bound.min.y}), lngLatToProjectedMeters({bound.max.x, bound.max.y}) };
}

} // namespace Tangram
