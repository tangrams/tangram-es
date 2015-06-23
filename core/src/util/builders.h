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
    
    PolyLineOptions() : cap(CapTypes::BUTT), join(JoinTypes::MITER) {};
    PolyLineOptions(CapTypes _c, JoinTypes _j) : cap(_c), join(_j) {};
};

typedef std::function<void(const glm::vec3& coord, const glm::vec3& normal, const glm::vec2& uv)> PolygonVertexFn;

typedef std::function<void(size_t reserve)> SizeHintFn;

struct PolygonOutput {
    std::vector<int>& indices; // indices for drawing the polyon as triangles are added to this vector
    PolygonVertexFn addVertex;
    SizeHintFn sizeHint;
    size_t numVertices = 0;
    PolygonOutput(std::vector<int>& indices, PolygonVertexFn addVertex, SizeHintFn sizeHint = SizeHintFn())
      : indices(indices),
        addVertex(addVertex),
        sizeHint(sizeHint){}
};

// coord  tesselated output coordinate
// normal extrusion vector of the output coordinate
// uv     texture coordinate of the output coordinate
typedef std::function<void(const glm::vec3& coord, const glm::vec2& normal, const glm::vec2& uv)> PolyLineVertexFn;

struct PolyLineOutput {
    std::vector<int>& indices; // indices for drawing the polyline as triangles are added to this vector
    PolyLineVertexFn addVertex;
    size_t numVertices = 0;

    PolyLineOutput(std::vector<int>& indices, PolyLineVertexFn addVertex) : indices(indices), addVertex(addVertex){}
};

class Builders {
    
public:
    
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
