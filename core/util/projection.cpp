#include <cmath>
#include <iostream>
#include <iomanip>
#include <stdio.h>
#include "projection.h"


MercatorProjection::MercatorProjection(int _tileSize) : MapProjection(ProjectionType::mercator), m_TileSize(_tileSize) {
    float invTileSize = 1.0/m_TileSize;
    m_Res = 2.0 * HALF_CIRCUMFERENCE * invTileSize;
}

glm::dvec2 MercatorProjection::LatLonToMeters(glm::dvec2 _latLon) {
    glm::dvec2 meters;
    meters.x = _latLon.y * HALF_CIRCUMFERENCE * INV_180;
    meters.y = log( tan( PI*0.25 + _latLon.x * PI * INV_360));
    meters.y = meters.y * (double)R_EARTH;
    return meters;
}

glm::dvec2 MercatorProjection::MetersToLatLon(glm::dvec2 _meters) {
    glm::dvec2 latLon;
    float invHalfCircum = 1.0/HALF_CIRCUMFERENCE;
    float invPI = 1.0/PI;
    latLon.y = _meters.x * invHalfCircum * 180.0;
    latLon.x = (2 * atan(exp( (_meters.y / R_EARTH ) )) - PI*0.5) * 180 * invPI;
    return latLon;
}

glm::dvec2 MercatorProjection::PixelsToMeters(glm::vec2 _pix, int _zoom) {
    glm::dvec2 meters;
    // resolution: meters/pixel for a given zoom level
    float res = m_Res * pow(2, -_zoom);
    meters.x = _pix.x * res - HALF_CIRCUMFERENCE;
    meters.y = _pix.y * res - HALF_CIRCUMFERENCE;
    return meters;
}

glm::vec2 MercatorProjection::MetersToPixel(glm::dvec2 _meters, int _zoom) {
    glm::vec2 pix;
    float res = m_Res * pow(2, -_zoom);
    float invRes = 1.0/res;
    pix.x = ( _meters.x + HALF_CIRCUMFERENCE ) * invRes;
    pix.y = ( _meters.y + HALF_CIRCUMFERENCE ) * invRes;
    return pix;
}

glm::ivec2 MercatorProjection::PixelsToTileXY(glm::vec2 _pix) {
    //returns the tile covering a region of a pixel
    glm::ivec2 tileXY;
    float invTileSize = 1.0/m_TileSize;
    tileXY.x = int(glm::ceil( _pix.x * invTileSize) - 1);
    tileXY.y = int(glm::ceil( _pix.y * invTileSize) - 1);
    return tileXY;
}

glm::ivec2 MercatorProjection::MetersToTileXY(glm::dvec2 _meters, int _zoom) {
    return PixelsToTileXY( MetersToPixel(_meters, _zoom));
}

glm::dvec2 MercatorProjection::TileXYToLatLon(glm::ivec3 _tileCoord) {
    glm::dvec2 latLon;
    float n = pow(2, -_tileCoord.z);
    float invPI = 1.0/PI;
    latLon.y = _tileCoord.x * n * 360.0 - 180.0;
    latLon.x = atanf(sinhf(PI * (1.0 - 2.0 * _tileCoord.y * n))) * 180.0 * invPI;
    return latLon;
}


glm::ivec2 MercatorProjection::LatLonToTileXY(glm::dvec2 _latLon, int zoom) {
    glm::ivec2 tileXY;
    float n = pow(2, zoom);
    float invPI = 1.0/PI;
    tileXY.x = n * ((_latLon.y + 180.0) * INV_360);
    tileXY.y = n * (1.0 - (glm::log(tanf(_latLon.x) + 1.0/cos(_latLon.x)) * invPI )) * 0.5;
    return tileXY;
}

glm::dvec4 MercatorProjection::TileBounds(glm::ivec3 _tileCoord) {
    glm::dvec2 boundMin, boundMax;
    glm::dvec4 latLonBound;
    latLonBound = TileLatLonBounds(_tileCoord);
    boundMin = LatLonToMeters(glm::dvec2(latLonBound.x, latLonBound.y));
    boundMax = LatLonToMeters(glm::dvec2(latLonBound.z, latLonBound.w));
    return glm::dvec4(boundMin.x, boundMin.y, boundMax.x, boundMax.y);
}

glm::dvec4 MercatorProjection::TileLatLonBounds(glm::ivec3 _tileCoord) {
    glm::dvec2 minBound, maxBound;
    minBound = TileXYToLatLon(_tileCoord);
    maxBound = TileXYToLatLon(glm::ivec3(_tileCoord.x+1, _tileCoord.y+1, _tileCoord.z));
    return glm::dvec4(minBound.x, minBound.y, maxBound.x, maxBound.y);
}

