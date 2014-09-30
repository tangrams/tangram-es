#include <cmath>

#include "projection.h"

MercProjection::MercProjection(int _tileSize) : MapProjection(ProjectionType::mercator), m_TileSize(_tileSize) {
    float invTileSize = 1.0/m_TileSize;
    m_Res = 2.0 * HALF_CIRCUMFERENCE * invTileSize;
}

glm::vec2 MercProjection::LatLonToMeters(glm::vec2 _latLon) {
    glm::vec2 meters;
    //Lon -> origin shift, as parallel lines
    meters.x = _latLon.y * HALF_CIRCUMFERENCE * INV_180;
    //Lat -> first do scaling as per mercator then origin shift
    meters.y = glm::log( glm::tan(90.0 + _latLon.x) * PI * INV_360) * PI * INV_180;
    meters.y = meters.y * HALF_CIRCUMFERENCE * INV_180;
    return meters;
}

glm::vec2 MercProjection::MetersToLatLon(glm::vec2 _meters) {
    glm::vec2 latLon;
    float invHalfCircum = 1.0/HALF_CIRCUMFERENCE;
    float invPI = 1.0/PI;
    latLon.y = _meters.x * invHalfCircum * 180.0;
    latLon.x = _meters.y * invHalfCircum * 180.0;
    latLon.x = 180.0 * invPI * (2.0 * glm::atan( glm::exp(latLon.x * PI * INV_180) ) - PI * 0.5);
    return latLon;
}

glm::vec2 MercProjection::PixelsToMeters(glm::vec2 _pix, int _zoom) {
    glm::vec2 meters;
    // resolution: meters/pixel for a given zoom level
    float res = m_Res * pow(2, -_zoom);
    meters.x = _pix.x * res - HALF_CIRCUMFERENCE;
    meters.y = _pix.y * res - HALF_CIRCUMFERENCE;
    return meters;
}

glm::vec2 MercProjection::MetersToPixel(glm::vec2 _meters, int _zoom) {
    glm::vec2 pix;
    float res = m_Res * pow(2, -_zoom);
    float invRes = 1.0/res;
    pix.x = ( _meters.x + HALF_CIRCUMFERENCE ) * invRes;
    pix.y = ( _meters.y + HALF_CIRCUMFERENCE ) * invRes;
    return pix;
}

glm::ivec2 MercProjection::PixelsToTileXY(glm::vec2 _pix) {
    //returns the tile covering a region of a pixel
    glm::ivec2 tileXY;
    float invTileSize = 1.0/m_TileSize;
    tileXY.x = int(glm::ceil( _pix.x * invTileSize) - 1);
    tileXY.y = int(glm::ceil( _pix.y * invTileSize) - 1);
    return tileXY;
}

glm::ivec2 MercProjection::MetersToTileXY(glm::vec2 _meters, int _zoom) {
    return PixelsToTileXY( MetersToPixel(_meters, _zoom));
}

glm::vec4 MercProjection::TileBounds(glm::ivec3 _tileCoord) {
    glm::vec2 boundMin, boundMax;
    boundMin = PixelsToMeters( glm::vec2(_tileCoord.x*m_TileSize, _tileCoord.y*m_TileSize), _tileCoord.z);
    boundMax = PixelsToMeters( glm::vec2((_tileCoord.x+1)*m_TileSize, (_tileCoord.y+1)*m_TileSize), _tileCoord.z);
    return glm::vec4(boundMin.x, boundMin.y, boundMax.x, boundMax.y);
}

glm::vec4 MercProjection::TileLatLonBounds(glm::ivec3 _tileCoord) {
    glm::vec4 bounds = TileBounds(_tileCoord);
    glm::vec2 minBound, maxBound;
    minBound = MetersToLatLon(glm::vec2(bounds.x, bounds.y));
    maxBound = MetersToLatLon(glm::vec2(bounds.z, bounds.w));
    return glm::vec4(minBound.x, minBound.y, maxBound.x, maxBound.y);
}

