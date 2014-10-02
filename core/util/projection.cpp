/*
 * Reference used for implementation: http://www.maptiler.org/google-maps-coordinates-tile-bounds-projection/
 */

#include <cmath>
#include <iostream>
#include <iomanip>
#include <stdio.h>
#include "projection.h"


MercatorProjection::MercatorProjection(int _tileSize) : MapProjection(ProjectionType::mercator), m_TileSize(_tileSize) {
    double invTileSize = 1.0/m_TileSize;
    m_Res = 2.0 * HALF_CIRCUMFERENCE * invTileSize;
}

glm::dvec2&& MercatorProjection::LonLatToMeters(glm::dvec2 _lonLat) {
    glm::dvec2 meters;
    meters.x = _lonLat.x * HALF_CIRCUMFERENCE * INV_180;
    meters.y = log( tan( PI*0.25 + _lonLat.y * PI * INV_360));
    meters.y = meters.y * (double)R_EARTH;
    return std::move(meters);
}

glm::dvec2&& MercatorProjection::MetersToLonLat(glm::dvec2 _meters) {
    glm::dvec2 lonLat;
    double invHalfCircum = 1.0/HALF_CIRCUMFERENCE;
    double invPI = 1.0/PI;
    lonLat.x = _meters.x * invHalfCircum * 180.0;
    lonLat.y = (2 * atan(exp( (_meters.y / R_EARTH ) )) - PI*0.5) * 180 * invPI;
    return std::move(lonLat);
}

glm::dvec2&& MercatorProjection::PixelsToMeters(glm::dvec2 _pix, int _zoom) {
    glm::dvec2 meters;
    double res = m_Res / (1 << _zoom);
    meters.x = _pix.x * res - HALF_CIRCUMFERENCE;
    meters.y = _pix.y * res - HALF_CIRCUMFERENCE;
    return std::move(meters);
}

glm::dvec2&& MercatorProjection::MetersToPixel(glm::dvec2 _meters, int _zoom) {
    glm::dvec2 pix;
    double invRes = (1 << _zoom) / m_Res;
    pix.x = ( _meters.x + HALF_CIRCUMFERENCE ) * invRes;
    pix.y = ( _meters.y + HALF_CIRCUMFERENCE ) * invRes;
    return std::move(pix);
}

glm::ivec2&& MercatorProjection::PixelsToTileXY(glm::dvec2 _pix) {
    //returns the tile covering a region of a pixel
    glm::ivec2 tileXY;
    double invTileSize = 1.0/m_TileSize;
    tileXY.x = int(ceil( _pix.x * invTileSize) - 1);
    tileXY.y = int(ceil( _pix.y * invTileSize) - 1);
    return std::move(tileXY);
}

glm::ivec2&& MercatorProjection::MetersToTileXY(glm::dvec2 _meters, int _zoom) {
    glm::dvec2 pix;
    glm::ivec2 tileXY;
    pix = MetersToPixel(_meters, _zoom);
    tileXY = PixelsToTileXY(pix);
    return std::move(tileXY);
}

glm::dvec2&& MercatorProjection::PixelsToRaster(glm::dvec2 _pix, int _zoom) {
    glm::dvec2 transformedPix;
    double mapSize;
    mapSize = m_TileSize << _zoom;
    transformedPix = glm::dvec2(_pix.x, (mapSize - _pix.y));
    return std::move(transformedPix);
}

glm::dvec4&& MercatorProjection::TileBounds(glm::ivec3 _tileCoord) {
    glm::dvec2 boundMin, boundMax;
    glm::dvec4 bounds;
    boundMin = PixelsToMeters(glm::vec2(_tileCoord.x*m_TileSize, _tileCoord.y*m_TileSize), _tileCoord.z);
    boundMax = PixelsToMeters(glm::vec2((_tileCoord.x+1)*m_TileSize, (_tileCoord.y+1)*m_TileSize), _tileCoord.z);
    bounds = glm::dvec4(boundMin.x, boundMin.y, boundMax.x, boundMax.y);
    return std::move(bounds);
}

glm::dvec4&& MercatorProjection::TileLonLatBounds(glm::ivec3 _tileCoord) {
    glm::dvec2 boundMin, boundMax;
    glm::dvec4 tileBounds, lonLatBounds;
    tileBounds = TileBounds(_tileCoord);
    boundMin = MetersToLonLat(glm::dvec2(tileBounds.x, tileBounds.y));
    boundMax = MetersToLonLat(glm::dvec2(tileBounds.z, tileBounds.w));
    lonLatBounds = glm::dvec4(boundMin.x, boundMin.y, boundMax.x, boundMax.y);
    return std::move(lonLatBounds);
}
