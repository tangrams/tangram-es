#pragma once

#include <vector>

#include "tileData.h"
#include "platform.h"

enum class CapTypes {
    BUTT = 0, // No points added to end of line
    SQUARE = 2, // Two points added to make a square extension
    ROUND = 6 // Six points added in a fan to make a round cap
};

enum class JoinTypes {
    MITER = 0, // No points added at line join
    BEVEL = 1, // One point added to flatten the corner of a join
    ROUND = 5 // Five points added in a fan to make a round outer join
};

struct PolyLineOptions {
    CapTypes cap;
    JoinTypes join;
    float halfWidth;
};

struct PolygonOutput {
    std::vector<glm::vec3>& points; // tesselated output coordinates are added to this vector
    std::vector<int>& indices; // indices for drawing the polyon as triangles are added to this vector
    std::vector<glm::vec3>& normals; // normal vectors for each output coordinate are added to this vector
    std::vector<glm::vec2>& texcoords; // if not null, 2D texture coordinates for each output coordinate are added to this vector
};

struct PolyLineOutput {
    std::vector<glm::vec3>& points; // tesselated output coordinates are added to this vector
    std::vector<int>& indices; // indices for drawing the polyline as triangles are added to this vector
    std::vector<glm::vec2>& scalingVecs; // if not null, 2D vectors for scaling the polyline are added to this vector
    std::vector<glm::vec2>& texcoords; // if not null, 2D texture coordinates for each output coordinate are added to this vector
};

class Builders {
    
public:
    
    static std::vector<glm::vec2> NO_TEXCOORDS;
    static std::vector<glm::vec2> NO_SCALING_VECS;
    
    /* Build a tesselated polygon
     * @_polygon input coordinates describing the polygon
     * @_out output vectors, see <PolygonOutput>
     */
    static void buildPolygon(const Polygon& _polygon, PolygonOutput& _out);

    /* Build extruded 'walls' from a polygon
     * @_polygon input coordinates describing the polygon
     * @_minHeight the extrusion will extend from this z coordinate to the z of the polygon points
     * @_out output vectors, see <PolygonOutput>
     */
    static void buildPolygonExtrusion(const Polygon& _polygon, const float& _minHeight, PolygonOutput& _out);

    /* Build a tesselated polygon line of fixed width from line coordinates
     * @_line input coordinates describing the line
     * @_options parameters for polyline construction
     * @_out output vectors, see <PolyLineOutput>
     */
    static void buildPolyLine(const Line& _line, const PolyLineOptions& _options, PolyLineOutput& _out);
    
    /* Build a tesselated outline that follows the given line while skipping tile boundaries */
    static void buildOutline(const Line& _line, const PolyLineOptions& _options, PolyLineOutput& _out);
    
    /* Build a tesselated square centered on a point coordinate
     * 
     * NOT IMPLEMENTED
     */
    static void buildQuadAtPoint(const Point& _pointIn, const glm::vec3& _normal, float width, float height, PolygonOutput& _out);
    
};
