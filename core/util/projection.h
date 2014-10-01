#pragma once

//Define global constants
#define PI 3.14159265358979323846264
#define R_EARTH 6378137.0

#include "glm/glm.hpp"

enum class ProjectionType {
    mercator
};

class MapProjection {
protected:
    /* m_type: type of map projection: example: mercator*/
    ProjectionType m_type;
public:
    constexpr static double INV_360 = 1.0/360.0;
    constexpr static double INV_180 = 1.0/180.0;
    constexpr static double HALF_CIRCUMFERENCE = PI * R_EARTH;
    MapProjection(ProjectionType _type) : m_type(_type) {};

    /*
     * LatLong to ProjectionType-Meter
     * Arguments:
     *   _latlon: glm::dvec2 having lat and lon info. x: lon, y: lat
     * Return value:
     *    meter (glm::dvec2).
     */
    virtual glm::dvec2 LatLonToMeters(glm::dvec2 _latLon) = 0;

    /* 
     * ProjectionType-Meters to Lat Lon
     *  Arguments: 
     *    _meter: glm::dvec2 having the projection units in meters. x: lon, y:lat
     *  Return value:
     *    latlon (glm::dvec2)
     */
    virtual glm::dvec2 MetersToLatLon(glm::dvec2 _meters) = 0;

    /*
     * Returns resolution in meters per pixel at equator.
     */
    static double resolution(int _zoom) {
        return m_Res / ( 1 << _zoom);
    }

    /*
     * Returns inverse of resolution
     */
    static double invResolution(int _zoom) {
        return (1 << _zoom) / m_Res;
    }

    /* 
     * Converts a pixel coordinate at a given zoom level of pyramid to projection meters
     * Screen pixels to Meters 
     *  Arguments:
     *    _pix: pixels defined as glm::vec2
     *    _zoom: zoom level to determine the projection-meters
     *  Return value:
     *    projection-meters (glm::dvec2)
     */
    virtual glm::dvec2 PixelsToMeters(glm::vec2 _pix, int _zoom) = 0;
    
    /* 
     * Converts projection meters to pyramid pixel coordinates in given zoom level.
     * Meters to Screen Pixels 
     * Arguments:
     *   _meters: projection-meters (glm::dvec2)
     *   _zoom: zoom level to determine pixels
     * Return Value:
     *   pixels (glm::vec2)
     */
    virtual glm::vec2 MetersToPixel(glm::dvec2 _meters, int _zoom) = 0;
    
    /*
     * Returns a tile covering region in given pixel coordinates.
     * Argument:
     *  _pix: pixel 
    virtual glm::ivec2 PixelsToTileXY(glm::vec2 _pix) = 0;
    
    /* Projection-meters to TILEXY
    *  Arguments:
    *    _meters: projection-meters (glm::dvec2)
    *    _zoom: zoom level for which tile coordinates need to be determined
    *  Return Value:
    *    Tile coordinates (x and y) (glm::ivec2)
    */
    virtual glm::ivec2 MetersToTileXY(glm::dvec2 _meters, int _zoom) = 0;
    
    /* bounds of space in projection-meters
    *  Arguments:
    *    _tileCoord: glm::ivec3 (x,y and zoom)
    *  Return value:
    *    bounds in projection-meters (glm::dvec4)
    *       x,y : min bounds in projection meters
    *       z,w : max bounds in projection meters
    */
    virtual glm::dvec4 TileBounds(glm::ivec3 _tileCoord) = 0;
    
    /* bounds of space in lat lon
    *  Arguments:
    *    _tileCoord: glm::ivec3 (x,y and zoom)
    *  Return value:
    *    bounds in lat lon (glm::vec4)
    *       x,y: min bounds in lat lon
    *       z,w: max bounds in lat lon
    */
    virtual glm::dvec4 TileLatLonBounds(glm::ivec3 _tileCoord) = 0;
    
    /*
     * TileXY to Lat/Lon
     */
    virtual glm::dvec2 TileXYToLatLon(glm::ivec3 _tileXY) = 0;
    
    /*
     * Lat/Lon to TileXY
     */
    virtual glm::ivec2 LatLonToTileXY(glm::dvec2 _latLon, int _zoom) = 0;
    
    /* Returns the projection type of a given projection instance 
    *   (example: ProjectionType::Mercator)
    */
    virtual ProjectionType GetMapProjectionType() {return m_type;}

    virtual ~MapProjection() {}
};

class MercatorProjection : public MapProjection {
    /* 
     * Following define the boundry covered by this mercator projection
     */
    int m_TileSize;
    float m_Res;
public:
    /*Constructor for MercatorProjection
    * _type: type of map projection, example ProjectionType::Mercator
    * _tileSize: size of the map tile, default is 256
    */
    MercatorProjection(int  _tileSize=256);

    virtual glm::dvec2 LatLonToMeters(glm::dvec2 _latLon) override;
    virtual glm::dvec2 MetersToLatLon(glm::dvec2 _meters) override;
    virtual glm::dvec2 PixelsToMeters(glm::vec2 _pix, int _zoom) override;
    virtual glm::vec2 MetersToPixel(glm::dvec2 _meters, int _zoom) override;
    virtual glm::ivec2 PixelsToTileXY(glm::vec2 _pix) override;
    virtual glm::ivec2 MetersToTileXY(glm::dvec2 _meters, int _zoom) override;
    virtual glm::dvec4 TileBounds(glm::ivec3 _tileCoord) override;
    virtual glm::dvec4 TileLatLonBounds(glm::ivec3 _tileCoord) override;
    virtual glm::dvec2 TileXYToLatLon(glm::ivec3 _tileXY);
    virtual glm::ivec2 LatLonToTileXY(glm::dvec2 _latLon, int _zoom);
    virtual ~MercatorProjection() {}
};

