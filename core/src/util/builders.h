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

/* Callback function for PolygonBuilder:
 *
 * @coord  tesselated output coordinate
 * @normal triangle plane normal
 * @uv     texture coordinate of the output coordinate
 */
typedef std::function<void(const glm::vec3& coord, const glm::vec3& normal, const glm::vec2& uv)> PolygonVertexFn;

typedef std::function<void(size_t reserve)> SizeHintFn;

/* PolygonBuilder context,
 * see Builders::buildPolygon() and Builders::buildPolygonExtrusion()
 */
struct PolygonBuilder {
    std::vector<int> indices; // indices for drawing the polyon as triangles are added to this vector
    PolygonVertexFn addVertex;
    SizeHintFn sizeHint;
    size_t numVertices = 0;
    bool useTexCoords;

    PolygonBuilder(PolygonVertexFn _addVertex, SizeHintFn _sizeHint, bool _useTexCoords = true)
        : addVertex(_addVertex), sizeHint(_sizeHint), useTexCoords(_useTexCoords){}
};


/* Callback function for PolyLineBuilder:
 *
 * @coord   tesselated output coordinate
 * @enormal extrusion vector of the output coordinate
 * @uv      texture coordinate of the output coordinate
 */
typedef std::function<void(const glm::vec3& coord, const glm::vec2& enormal, const glm::vec2& uv)> PolyLineVertexFn;

/* PolyLineBuilder context,
 * see Builders::buildPolyLine()
 */
struct PolyLineBuilder {
    PolyLineOptions options;
    std::vector<int> indices; // indices for drawing the polyline as triangles are added to this vector
    PolyLineVertexFn addVertex;
    size_t numVertices = 0;

    PolyLineBuilder(PolyLineVertexFn _addVertex, PolyLineOptions _options = PolyLineOptions())
        : options(_options), addVertex(_addVertex){}
};

/* Callback function for SpriteBuilder
 * @coord   tesselated coordinates of the sprite quad in screen space
 * @uv      texture coordinate of the ouptput coordinate
 */
typedef std::function<void(const glm::vec2& screencoord, const glm::vec2& uv)> SpriteBuilderFn;

/* SpriteBuidler context
 */
struct SpriteBuilder {
    std::vector<int> indices;
    SpriteBuilderFn addVertex;
    size_t numVerts = 0;
    
    SpriteBuilder(SpriteBuilderFn _addVertex) : addVertex(_addVertex) {}
};

class Builders {
    
public:
    
    /* Build a tesselated polygon
     * @_polygon input coordinates describing the polygon
     * @_ctx output vectors, see <PolygonBuilder>
     */
    static void buildPolygon(const Polygon& _polygon, PolygonBuilder& _ctx);

    /* Build extruded 'walls' from a polygon
     * @_polygon input coordinates describing the polygon
     * @_minHeight the extrusion will extend from this z coordinate to the z of the polygon points
     * @_ctx output vectors, see <PolygonBuilder>
     */
    static void buildPolygonExtrusion(const Polygon& _polygon, const float& _minHeight, PolygonBuilder& _ctx);

    /* Build a tesselated polygon line of fixed width from line coordinates
     * @_line input coordinates describing the line
     * @_options parameters for polyline construction
     * @_ctx output vectors, see <PolyLineBuilder>
     */
    static void buildPolyLine(const Line& _line, PolyLineBuilder& _ctx);
    
    /* Build a tesselated outline that follows the given line while skipping tile boundaries */
    static void buildOutline(const Line& _line, PolyLineBuilder& _ctx);
    
    /* Build a tesselated quad centered on _origin
     * @_screenPos the sprite origin in screen space
     * @_spriteOrigin the sprite origin in the texture sprite atlas
     * @_spriteSize the sprite size in the texture sprite atlas
     * @_atlasSize the sprite atlas size
     * @_ctx output vectors, see <SpriteBuilder>
     */
    static void buildSpriteQuadAtPoint(const glm::vec2& _screenOrigin, const glm::vec2& _spriteOrigin, const glm::vec2& _spriteSize, const glm::vec2& _atlasSize, SpriteBuilder& _ctx);
    
};
