#pragma once

#include <vector>

#include "tileData.h"
#include "platform.h"

namespace Builders {
    
    enum class CapTypes {
        BUTT,
        SQUARE,
        ROUND
    };
    
    enum class JoinTypes {
        MITER,
        BEVEL,
        ROUND
    };
    
    struct PolyLineStyle {
        
        PolyLineStyle() :
            cap(CapTypes::BUTT), join(JoinTypes::MITER), halfWidth(0.05), closePolygon(false), removeTileEdges(false) {}
        
        PolyLineStyle(CapTypes _cap, JoinTypes _join, float _hw, bool _close, bool _rmEdges) :
            cap(_cap), join(_join), halfWidth(_hw), closePolygon(_close), removeTileEdges(_rmEdges) {}
        
        CapTypes cap;
        JoinTypes join;
        float halfWidth;
        bool closePolygon;
        bool removeTileEdges;
    };

    /* Build a tesselated polygon
     * @_polygon input coordinates describing the polygon
     * @_pointsOut tesselated output coordinates are added to this vector
     * @_normalsOut normal vectors for each output coordinate are added to this vector
     * @_indicesOut indices for drawing the polygon as triangles are added to this vector
     */
    void buildPolygon(const Polygon& _polygon, std::vector<glm::vec3>& _pointsOut, std::vector<glm::vec3>& _normalsOut, std::vector<int>& _indicesOut);
    
    /* Build a tesselated polygon with texture coordinates
     * @_polygon input coordinates describing the polygon
     * @_pointsOut tesselated output coordinates are added to this vector
     * @_normalsOut normal vectors for each output coordinate are added to this vector
     * @_indicesOut indices for drawing the polygon as triangles are added to this vector
     * @_texcoordsOut 2D texture coordinates for each output coordinate are added to this vector
     */
    void buildPolygon(const Polygon& _polygon, std::vector<glm::vec3>& _pointsOut, std::vector<glm::vec3>& _normalsOut, std::vector<int>& _indicesOut, std::vector<glm::vec2>& _texcoordsOut);

    /* Build extruded 'walls' from a polygon
     * @_polygon input coordinates describing the polygon
     * @_minHeight the extrusion will extend from this z coordinate to the z of the polygon points
     * @_pointsOut tesselated output coordinates are added to this vector
     * @_normalsOut normal vectors for each output coordinate are added to this vector
     * @_indicesOut indices for drawing the extrusion as triangles are added to this vector
     */
    void buildPolygonExtrusion(const Polygon& _polygon, const float& _minHeight, std::vector<glm::vec3>& _pointsOut, std::vector<glm::vec3>& _normalsOut, std::vector<int>& _indicesOut);
    
    /* Build extruded 'walls' from a polygon with texture coordinates
     * @_polygon input coordinates describing the polygon
     * @_minHeight the extrusion will extend from this z coordinate to the z of the polygon points
     * @_pointsOut tesselated output coordinates are added to this vector
     * @_normalsOut normal vectors for each output coordinate are added to this vector
     * @_indicesOut indices for drawing the extrusion as triangles are added to this vector
     * @_texcoordsOut 2D texture coordinates for each output coordinate are added to this vector
     */
    void buildPolygonExtrusion(const Polygon& _polygon, const float& _minHeight, std::vector<glm::vec3>& _pointsOut, std::vector<glm::vec3>& _normalsOut, std::vector<int>& _indicesOut, std::vector<glm::vec2>& _texcoordsOut);

    /* Build a tesselated polygon line of fixed width from line coordinates
     * @_line input coordinates describing the line
     * @_halfWidth half the width of a straight segment of the tesselated line
     * @_pointsOut tesselated output coordinates are added to this vector
     * @_indicesOut indices for drawing the polyline as triangles are added to this vector
     */
    void buildPolyLine(const Line& _line, std::vector<glm::vec3>& _pointsOut, std::vector<int>& _indicesOut, const PolyLineStyle& _style);
    
    /* Build a tesselated polygon line of fixed width with texture coordinates from line coordinates
     * @_line input coordinates describing the line
     * @_halfWidth half the width of a straight segment of the tesselated line
     * @_pointsOut tesselated output coordinates are added to this vector
     * @_indicesOut indices for drawing the polyline as triangles are added to this vector
     * @_texcoordsOut 2D texture coordinates for each output coordinate are added to this vector
     */
    void buildPolyLine(const Line& _line, std::vector<glm::vec3>& _pointsOut, std::vector<int>& _indicesOut, std::vector<glm::vec2>& _texcoordsOut, const PolyLineStyle& _style);
    
    /* Build a tesselated, scalable polygon line from line coordinates
     * @_line input coordinates describing the line
     * @_pointsOut tesselated output coordinates are added to this vector
     * @_scalingVecsOut 2D vectors along which to scale the polyline are added to this vector; similar to normal vectors
     * @_indicesOut indices for drawing the polyline as triangles are added to this vector
     */
    void buildScalablePolyLine(const Line& _line, std::vector<glm::vec3>& _pointsOut, std::vector<glm::vec2>& _scalingVecsOut, std::vector<int>& _indicesOut, const PolyLineStyle& _style);
    
    /* Build a tesselated, scalable polygon line with texture coordinate from line coordinates
     * @_line input coordinates describing the line
     * @_pointsOut tesselated output coordinates are added to this vector
     * @_scalingVecsOut 2D vectors along which to scale the polyline are added to this vector; similar to normal vectors
     * @_indicesOut indices for drawing the polyline as triangles are added to this vector
     * @_texcoordsOut 2D texture coordinates for each output coordinate are added to this vector
     */
    void buildScalablePolyLine(const Line& _line, std::vector<glm::vec3>& _pointsOut, std::vector<glm::vec2>& _scalingVecsOut, std::vector<int>& _indicesOut, std::vector<glm::vec2>& _texcoordsOut, const PolyLineStyle& _style);
    
    /* Build a tesselated square centered on a point coordinate
     * 
     * NOT IMPLEMENTED
     */
    void buildQuadAtPoint(const Point& _pointIn, const glm::vec3& _normal, float width, float height, std::vector<glm::vec3>& _pointsOut, std::vector<int>& _indicesOut);
    
}
