#pragma one

//Define global constants
#define PI 3.1415926535
#define HALF_CIRCUMFERENCE 20037508.342789244
#define INV_180 0.005555555
#define INV_360 0.002777777


#include "glm/glm.hpp"

const glm::vec2 ORIGIN(HALF_CIRCUMFERENCE, HALF_CIRCUMFERENCE);

enum class ProjectionType {
    mercator
};

class MapProjection {
protected:
    /* m_type: type of map projection: example: mercator*/
    ProjectionType m_type;
public:
    MapProjection(ProjectionType _type) : m_type(_type) {};
    
    /*LatLong to ProjectionType-Meter
    * Arguments:
    *   _latlon: glm::vec2 having lat and lon info. x: lon, y: lat
    * Return value:
    *    meter (glm::vec2).
    */
    virtual glm::vec2 LatLonToMeters(glm::vec2 _latLon) = 0;

    /* ProjectionType-Meters to Lat Lon
    *  Arguments: 
    *    _meter: glm::vec2 having the projection units in meters. x: lon, y:lat
    *  Return value:
    *    latlon (glm::vec2)
    */
    virtual glm::vec2 MetersToLatLon(glm::vec2 _meters) = 0;

    /* Screen pixels to Meters 
    * (TODO: Update description based on usage in the native app, might be moved to viewModule
    *  Arguments:
    *    _pix: screen pixels defined as glm::vec2
    *    _zoom: zoom level to determine the projection-meters
    *  Return value:
    *    projection-meters (glm::vec2)
    */
    virtual glm::vec2 PixelsToMeters(glm::vec2 _pix, int _zoom) = 0;
    
    /* Meters to Screen Pixels 
    * (TODO: Update description based on usage in the native app, might be moved to viewModule
    * Arguments:
    *   _meters: projection-meters (glm::vec2)
    *   _zoom: zoom level to determine screen pixels
    * Return Value:
    *   screen pixels (glm::vec2)
    */
    virtual glm::vec2 MetersToPixel(glm::vec2 _meters, int _zoom) = 0;
    
    /* TODO: Define when more clear on the use case for this. Might be moved to viewModule
    */
    virtual glm::ivec2 PixelsToTileXY(glm::vec2 _pix) = 0;
    
    /* Projection-meters to TILEXY
    *  Arguments:
    *    _meters: projection-meters (glm::vec2)
    *    _zoom: zoom level for which tile coordinates need to be determined
    *  Return Value:
    *    Tile coordinates (x and y) (glm::ivec2)
    */
    virtual glm::ivec2 MetersToTileXY(glm::vec2 _meters, int _zoom) = 0;
    
    /* bounds of space in projection-meters
    *  Arguments:
    *    _tileCoord: glm::ivec3 (x,y and zoom)
    *  Return value:
    *    bounds in projection-meters (glm::vec4)
    *       x,y : min bounds in projection meters
    *       z,w : max bounds in projection meters
    */
    virtual glm::vec4 TileBounds(glm::ivec3 _tileCoord) = 0;
    
    /* bounds of space in lat lon
    *  Arguments:
    *    _tileCoord: glm::ivec3 (x,y and zoom)
    *  Return value:
    *    bounds in lat lon (glm::vec4)
    *       x,y: min bounds in lat lon
    *       z,w: max bounds in lat lon
    */
    virtual glm::vec4 TileLatLonBounds(glm::ivec3 _tileCoord) = 0;
    
    /* Returns the projection type of a given projection instance 
    *   (example: ProjectionType::Mercator)
    */
    virtual ProjectionType GetMapProjectionType() {return m_type;}
    virtual ~MapProjection() {}
};

class MercProjection : public MapProjection {
    /* 
     * Following define the boundry covered by this mercator projection
     */
    float m_TileSize;
    float m_Res;
public:
    /*Constructor for MercProjection
    * _type: type of map projection, example ProjectionType::Mercator
    * _tileSize: size of the map tile, default is 256
    */
    MercProjection(ProjectionType _type, int  _tileSize=256);

    virtual glm::vec2 LatLonToMeters(glm::vec2 _latLon) override;
    virtual glm::vec2 MetersToLatLon(glm::vec2 _meters) override;
    virtual glm::vec2 PixelsToMeters(glm::vec2 _pix, int _zoom) override;
    virtual glm::vec2 MetersToPixel(glm::vec2 _meters, int _zoom) override;
    virtual glm::ivec2 PixelsToTileXY(glm::vec2 _pix) override;
    virtual glm::ivec2 MetersToTileXY(glm::vec2 _meters, int _zoom) override;
    virtual glm::vec4 TileBounds(glm::ivec3 _tileCoord) override;
    virtual glm::vec4 TileLatLonBounds(glm::ivec3 _tileCoord) override;
    virtual ~MercProjection() {}
};

