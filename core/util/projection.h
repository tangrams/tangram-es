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
    double m_Res;
public:
    constexpr static double INV_360 = 1.0/360.0;
    constexpr static double INV_180 = 1.0/180.0;
    constexpr static double HALF_CIRCUMFERENCE = PI * R_EARTH;
    /*
     * 256 is default tile size
     */
    MapProjection(ProjectionType _type) : m_type(_type) {};

    /*
     * LonLat to ProjectionType-Meter
     * Arguments:
     *   _lonlat: glm::dvec2 having lon and lat info
     * Return value:
     *    meter (glm::dvec2).
     */
    virtual glm::dvec2&& LonLatToMeters(glm::dvec2 _lonLat) = 0;

    /* 
     * ProjectionType-Meters to Lon Lat
     *  Arguments: 
     *    _meter: glm::dvec2 having the projection units in meters
     *  Return value:
     *    lonlat (glm::dvec2)
     */
    virtual glm::dvec2&& MetersToLonLat(glm::dvec2 _meters) = 0;

    /* 
     * Converts a pixel coordinate at a given zoom level of pyramid to projection meters
     * Screen pixels to Meters 
     *  Arguments:
     *    _pix: pixels defined as glm::dvec2
     *    _zoom: zoom level to determine the projection-meters
     *  Return value:
     *    projection-meters (glm::dvec2)
     */
    virtual glm::dvec2&& PixelsToMeters(glm::dvec2 _pix, int _zoom) = 0;
    
    /* 
     * Converts projection meters to pyramid pixel coordinates in given zoom level.
     * Meters to Screen Pixels 
     * Arguments:
     *   _meters: projection-meters (glm::dvec2)
     *   _zoom: zoom level to determine pixels
     * Return Value:
     *   pixels (glm::dvec2)
     */
    virtual glm::dvec2&& MetersToPixel(glm::dvec2 _meters, int _zoom) = 0;
    
    /*
     * Returns a tile covering region in given pixel coordinates.
     * Argument:
     *   _pix: pixel 
     * Return Value:
     *   Tile coordinates (x and y) glm::ivec2
     */
    virtual glm::ivec2&& PixelsToTileXY(glm::dvec2 _pix) = 0;
    
    /* 
     * Returns tile for given projection coordinates.
     *  Arguments:
     *    _meters: projection-meters (glm::dvec2)
     *    _zoom: zoom level for which tile coordinates need to be determined
     *  Return Value:
     *    Tile coordinates (x and y) (glm::ivec2)
     */
    virtual glm::ivec2&& MetersToTileXY(glm::dvec2 _meters, int _zoom) = 0;

    /*
     * Move the origin of pixel coordinates to top-left corner
     * Arguments:
     *   _pix: pixels
     *   _zoom: zoom level
     * Return Value:
     *   glm::dvec2 newPixels at top-left corner
     */
    virtual glm::dvec2&& PixelsToRaster(glm::dvec2 _pix, int _zoom) = 0;
    
    /* 
     * Returns bounds of the given tile
     *  Arguments:
     *    _tileCoord: glm::ivec3 (x,y and zoom)
     *  Return value:
     *    bounds in projection-meters (glm::dvec4)
     *       x,y : min bounds in projection meters
     *       z,w : max bounds in projection meters
     */
    virtual glm::dvec4&& TileBounds(glm::ivec3 _tileCoord) = 0;
    
    /* 
     * bounds of space in lon lat
     *  Arguments:
     *    _tileCoord: glm::ivec3 (x,y and zoom)
     *  Return value:
     *    bounds in lon lat (glm::dvec4)
     *       x,y: min bounds in lon lat
     *       z,w: max bounds in lon lat
     */
    virtual glm::dvec4&& TileLonLatBounds(glm::ivec3 _tileCoord) = 0;
    
    /* 
     * Returns the projection type of a given projection instance 
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
public:
    /*
     * Constructor for MercatorProjection
     * _type: type of map projection, example ProjectionType::Mercator
     * _tileSize: size of the map tile, default is 256
     */
    MercatorProjection(int  _tileSize=256);

    virtual glm::dvec2&& LonLatToMeters(glm::dvec2 _lonLat) override;
    virtual glm::dvec2&& MetersToLonLat(glm::dvec2 _meters) override;
    virtual glm::dvec2&& PixelsToMeters(glm::dvec2 _pix, int _zoom) override;
    virtual glm::dvec2&& MetersToPixel(glm::dvec2 _meters, int _zoom) override;
    virtual glm::ivec2&& PixelsToTileXY(glm::dvec2 _pix) override;
    virtual glm::ivec2&& MetersToTileXY(glm::dvec2 _meters, int _zoom) override;
    virtual glm::dvec2&& PixelsToRaster(glm::dvec2 _pix, int _zoom);
    virtual glm::dvec4&& TileBounds(glm::ivec3 _tileCoord) override;
    virtual glm::dvec4&& TileLonLatBounds(glm::ivec3 _tileCoord) override;
    //virtual glm::dvec2&& TileXYToLonLat(glm::ivec3 _tileXY);
    //virtual glm::ivec2&& LonLatToTileXY(glm::dvec2 _lonLat, int _zoom);
    virtual ~MercatorProjection() {}
};

