#pragma one

//Define global constants
#define PI 3.1415926535
#define HALF_CIRCUMFERENCE 20037508.342789244

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
    virtual glm::vec2 LatLonToMeters(glm::vec2 _latLon) = 0;
    virtual glm::vec2 MetersToLatLon(glm::vec2 _meters) = 0;
    virtual glm::vec2 PixelsToMeters(glm::vec2 _pix, int _zoom) = 0;
    virtual glm::vec2 MetersToPixel(glm::vec2 _meters, int _zoom) = 0;
    virtual glm::ivec2 PixelsToTileXY(glm::vec2 _pix) = 0;
    virtual glm::ivec2 MetersToTileXY(glm::vec2 _meters, int _zoom) = 0;
    virtual glm::vec4 TileBounds(glm::ivec3 _tileCoord) = 0;
    virtual glm::vec4 TileLatLonBounds(glm::ivec3 _tileCoord) = 0;
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
    MercProjection(ProjectionType _type, int  _tileSize=256);
    virtual glm::vec2 LatLonToMeters(glm::vec2 _latLon);
    virtual glm::vec2 MetersToLatLon(glm::vec2 _meters);
    virtual glm::vec2 PixelsToMeters(glm::vec2 _pix, int _zoom);
    virtual glm::vec2 MetersToPixel(glm::vec2 _meters, int _zoom);
    virtual glm::ivec2 PixelsToTileXY(glm::vec2 _pix);
    virtual glm::ivec2 MetersToTileXY(glm::vec2 _meters, int _zoom);
    virtual glm::vec4 TileBounds(glm::ivec3 _tileCoord);
    virtual glm::vec4 TileLatLonBounds(glm::ivec3 _tileCoord);
    virtual ~MercProjection() {}
};

