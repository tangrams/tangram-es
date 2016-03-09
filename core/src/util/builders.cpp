#include "builders.h"

#include "geom.h"
#include "glm/gtx/rotate_vector.hpp"
#include "platform.h"
#include <memory>

namespace mapbox { namespace util {
template <>
struct nth<0, Tangram::Point> {
    inline static float get(const Tangram::Point &t) { return t.x; };
};
template <>
struct nth<1, Tangram::Point> {
    inline static float get(const Tangram::Point &t) { return t.y; };
};
}}

namespace Tangram {

CapTypes CapTypeFromString(const std::string& str) {
    if (str == "square") { return CapTypes::square; }
    if (str == "round") { return CapTypes::round; }
    return CapTypes::butt;
}

JoinTypes JoinTypeFromString(const std::string& str) {
    if (str == "bevel") { return JoinTypes::bevel; }
    if (str == "round") { return JoinTypes::round; }
    return JoinTypes::miter;
}

void Builders::buildPolygon(const Polygon& _polygon, float _height, PolygonBuilder& _ctx) {

    _ctx.earcut(_polygon);

    uint16_t vertexDataOffset = _ctx.numVertices;

    if (vertexDataOffset == 0) {
        _ctx.indices = std::move(_ctx.earcut.indices);
    } else {
        _ctx.indices.reserve(_ctx.indices.size() +  _ctx.earcut.indices.size());
        for (auto i : _ctx.earcut.indices) {
            _ctx.indices.push_back(vertexDataOffset + i);
        }
    }

    glm::vec2 min, max;

    if (_ctx.useTexCoords) {
        min = glm::vec2(std::numeric_limits<float>::max());
        max = glm::vec2(std::numeric_limits<float>::min());

        for (auto& p : _polygon[0]) {
            min.x = std::min(min.x, p.x);
            min.y = std::min(min.y, p.y);
            max.x = std::max(max.x, p.x);
            max.y = std::max(max.y, p.y);
        }
    }

    // call the tesselator
    glm::vec3 normal(0.0, 0.0, 1.0);

    _ctx.numVertices += _ctx.earcut.vertices.size();
    _ctx.sizeHint(_ctx.numVertices);

    for (auto& p : _ctx.earcut.vertices) {
        glm::vec3 coord(p[0], p[1], _height);

        if (_ctx.useTexCoords) {
            glm::vec2 uv(mapValue(coord.x, min.x, max.x, 0., 1.),
                         mapValue(coord.y, min.y, max.y, 1., 0.));

            _ctx.addVertex(coord, normal, uv);
        } else {
            _ctx.addVertex(coord, normal, glm::vec2(0));
        }
    }
}

void Builders::buildPolygonExtrusion(const Polygon& _polygon, float _minHeight, float _maxHeight, PolygonBuilder& _ctx) {

    int vertexDataOffset = (int)_ctx.numVertices;

    glm::vec3 upVector(0.0f, 0.0f, 1.0f);
    glm::vec3 normalVector;

    size_t sumIndices = _ctx.indices.size();
    size_t sumVertices = _ctx.numVertices;

    for (auto& line : _polygon) {
        size_t lineSize = line.size();
        sumIndices += lineSize * 6;
        sumVertices += (lineSize - 1) * 4;

        _ctx.numVertices = sumVertices;
    }

    _ctx.indices.reserve(sumIndices);
    _ctx.sizeHint(sumVertices);

    for (auto& line : _polygon) {

        size_t lineSize = line.size();

        for (size_t i = 0; i < lineSize - 1; i++) {

            glm::vec3 a(line[i]);
            glm::vec3 b(line[i+1]);

            normalVector = glm::cross(upVector, b - a);
            normalVector = glm::normalize(normalVector);

            // 1st vertex top
            a.z = _maxHeight;
            _ctx.addVertex(a, normalVector, glm::vec2(1.,0.));

            // 2nd vertex top
            b.z = _maxHeight;
            _ctx.addVertex(b, normalVector, glm::vec2(0.,0.));

            // 1st vertex bottom
            a.z = _minHeight;
            _ctx.addVertex(a, normalVector, glm::vec2(1.,1.));

            // 2nd vertex bottom
            b.z = _minHeight;
            _ctx.addVertex(b, normalVector, glm::vec2(0.,1.));

            // Start the index from the previous state of the vertex Data
            _ctx.indices.push_back(vertexDataOffset);
            _ctx.indices.push_back(vertexDataOffset + 1);
            _ctx.indices.push_back(vertexDataOffset + 2);

            _ctx.indices.push_back(vertexDataOffset + 1);
            _ctx.indices.push_back(vertexDataOffset + 3);
            _ctx.indices.push_back(vertexDataOffset + 2);

            vertexDataOffset += 4;
        }
    }
}

// Get 2D perpendicular of two points
glm::vec2 perp2d(const glm::vec3& _v1, const glm::vec3& _v2 ){
    return glm::vec2(_v2.y - _v1.y, _v1.x - _v2.x);
}

// Helper function for polyline tesselation
inline void addPolyLineVertex(const glm::vec3& _coord, const glm::vec2& _normal, const glm::vec2& _uv, PolyLineBuilder& _ctx) {
    _ctx.numVertices++;
    _ctx.addVertex(_coord, _normal, _uv);
}

// Helper function for polyline tesselation; adds indices for pairs of vertices arranged like a line strip
void indexPairs( int _nPairs, int _nVertices, std::vector<uint16_t>& _indicesOut) {
    for (int i = 0; i < _nPairs; i++) {
        _indicesOut.push_back(_nVertices - 2*i - 4);
        _indicesOut.push_back(_nVertices - 2*i - 2);
        _indicesOut.push_back(_nVertices - 2*i - 3);

        _indicesOut.push_back(_nVertices - 2*i - 3);
        _indicesOut.push_back(_nVertices - 2*i - 2);
        _indicesOut.push_back(_nVertices - 2*i - 1);
    }
}

//  Tessalate a fan geometry between points A       B
//  using their normals from a center        \ . . /
//  and interpolating their UVs               \ p /
//                                             \./
//                                              C
void addFan(const glm::vec3& _pC,
            const glm::vec2& _nA, const glm::vec2& _nB, const glm::vec2& _nC,
            const glm::vec2& _uA, const glm::vec2& _uB, const glm::vec2& _uC,
            int _numTriangles, PolyLineBuilder& _ctx) {

    // Find angle difference
    float cross = _nA.x * _nB.y - _nA.y * _nB.x; // z component of cross(_CA, _CB)
    float angle = atan2f(cross, glm::dot(_nA, _nB));

    int startIndex = _ctx.numVertices;

    // Add center vertex
    addPolyLineVertex(_pC, _nC, _uC, _ctx);

    // Add vertex for point A
    addPolyLineVertex(_pC, _nA, _uA, _ctx);

    // Add radial vertices
    glm::vec2 radial = _nA;
    for (int i = 0; i < _numTriangles; i++) {
        float frac = (i + 1)/(float)_numTriangles;
        radial = glm::rotate(_nA, angle * frac);

        glm::vec2 uv(0.0);
        if (_ctx.useTexCoords) {
            uv = (1.f - frac) * _uA + frac * _uB;
        }

        addPolyLineVertex(_pC, radial, uv, _ctx);

        // Add indices
        _ctx.indices.push_back(startIndex); // center vertex
        _ctx.indices.push_back(startIndex + i + (angle > 0 ? 1 : 2));
        _ctx.indices.push_back(startIndex + i + (angle > 0 ? 2 : 1));
    }

}

// Function to add the vertices for line caps
void addCap(const glm::vec3& _coord, const glm::vec2& _normal, int _numCorners, bool _isBeginning, PolyLineBuilder& _ctx) {

    float v = _isBeginning ? 0.f : 1.f; // length-wise tex coord

    if (_numCorners < 1) {
        // "Butt" cap needs no extra vertices
        return;
    } else if (_numCorners == 2) {
        // "Square" cap needs two extra vertices
        glm::vec2 tangent(-_normal.y, _normal.x);
        addPolyLineVertex(_coord, _normal + tangent, {0.f, v}, _ctx);
        addPolyLineVertex(_coord, -_normal + tangent, {0.f, v}, _ctx);
        if (!_isBeginning) { // At the beginning of a line we can't form triangles with previous vertices
            indexPairs(1, _ctx.numVertices, _ctx.indices);
        }
        return;
    }

    // "Round" cap type needs a fan of vertices
    glm::vec2 nA(_normal), nB(-_normal), nC(0.f, 0.f), uA(1.f, v), uB(0.f, v), uC(0.5f, v);
    if (_isBeginning) {
        nA *= -1.f; // To flip the direction of the fan, we negate the normal vectors
        nB *= -1.f;
        uA.x = 0.f; // To keep tex coords consistent, we must reverse these too
        uB.x = 1.f;
    }
    addFan(_coord, nA, nB, nC, uA, uB, uC, _numCorners, _ctx);
}

// Tests if a line segment (from point A to B) is outside the edge of a tile
bool isOutsideTile(const glm::vec3& _a, const glm::vec3& _b) {

    // tweak this adjust if catching too few/many line segments near tile edges
    // TODO: make tolerance configurable by source if necessary
    float tolerance = 0.0005;
    float tile_min = 0.0 + tolerance;
    float tile_max = 1.0 - tolerance;

    if ( (_a.x < tile_min && _b.x < tile_min) ||
         (_a.x > tile_max && _b.x > tile_max) ||
         (_a.y < tile_min && _b.y < tile_min) ||
         (_a.y > tile_max && _b.y > tile_max) ) {
        return true;
    }

    return false;
}

void buildPolyLineSegment(const Line& _line, PolyLineBuilder& _ctx) {

    int lineSize = (int)_line.size();
    if (lineSize < 2) { return; }

    glm::vec3 coordPrev(_line[0]), coordCurr(_line[0]), coordNext(_line[1]);
    glm::vec2 normPrev, normNext, miterVec;

    int cornersOnCap = (int)_ctx.cap;
    int trianglesOnJoin = (int)_ctx.join;

    // Process first point in line with an end cap
    normNext = glm::normalize(perp2d(coordCurr, coordNext));
    addCap(coordCurr, normNext, cornersOnCap, true, _ctx);
    addPolyLineVertex(coordCurr, normNext, {1.0f, 0.0f}, _ctx); // right corner
    addPolyLineVertex(coordCurr, -normNext, {0.0f, 0.0f}, _ctx); // left corner

    // Process intermediate points
    for (int i = 1; i < lineSize - 1; i++) {

        coordPrev = coordCurr;
        coordCurr = coordNext;
        coordNext = _line[i + 1];

        normPrev = normNext;
        normNext = glm::normalize(perp2d(coordCurr, coordNext));

        // Compute "normal" for miter joint
        miterVec = normPrev + normNext;
        float scale = sqrtf(2.0f / (1.0f + glm::dot(normPrev, normNext)) / glm::dot(miterVec, miterVec) );
        miterVec *= scale;
        if (glm::length(miterVec) > _ctx.miterLimit) {
            trianglesOnJoin = 1;
        }

        float v = i / (float)lineSize;

        if (trianglesOnJoin == 0) {
            // Join type is a simple miter

            addPolyLineVertex(coordCurr, miterVec, {1.0, v}, _ctx); // right corner
            addPolyLineVertex(coordCurr, -miterVec, {0.0, v}, _ctx); // left corner
            indexPairs(1, _ctx.numVertices, _ctx.indices);

        } else {
            // Join type is a fan of triangles

            bool isRightTurn = (normNext.x * normPrev.y - normNext.y * normPrev.x) > 0; // z component of cross(normNext, normPrev)

            if (isRightTurn) {

                addPolyLineVertex(coordCurr, miterVec, {1.0f, v}, _ctx); // right (inner) corner
                addPolyLineVertex(coordCurr, -normPrev, {0.0f, v}, _ctx); // left (outer) corner
                indexPairs(1, _ctx.numVertices, _ctx.indices);

                addFan(coordCurr, -normPrev, -normNext, miterVec, {0.f, v}, {0.f, v}, {1.f, v}, trianglesOnJoin, _ctx);

                addPolyLineVertex(coordCurr, miterVec, {1.0f, v}, _ctx); // right (inner) corner
                addPolyLineVertex(coordCurr, -normNext, {0.0f, v}, _ctx); // left (outer) corner

            } else {

                addPolyLineVertex(coordCurr, normPrev, {1.0f, v}, _ctx); // right (outer) corner
                addPolyLineVertex(coordCurr, -miterVec, {0.0f, v}, _ctx); // left (inner) corner
                indexPairs(1, _ctx.numVertices, _ctx.indices);

                addFan(coordCurr, normPrev, normNext, -miterVec, {1.f, v}, {1.f, v}, {0.0f, v}, trianglesOnJoin, _ctx);

                addPolyLineVertex(coordCurr, normNext, {1.0f, v}, _ctx); // right (outer) corner
                addPolyLineVertex(coordCurr, -miterVec, {0.0f, v}, _ctx); // left (inner) corner
            }
        }
    }

    // Process last point in line with a cap
    addPolyLineVertex(coordNext, normNext, {1.f, 1.f}, _ctx); // right corner
    addPolyLineVertex(coordNext, -normNext, {0.f, 1.f}, _ctx); // left corner
    indexPairs(1, _ctx.numVertices, _ctx.indices);
    addCap(coordNext, normNext, cornersOnCap , false, _ctx);

}

void Builders::buildPolyLine(const Line& _line, PolyLineBuilder& _ctx) {

    if (_ctx.keepTileEdges) {

        buildPolyLineSegment(_line, _ctx);

    } else {

        int cut = 0;

        for (size_t i = 0; i < _line.size() - 1; i++) {
            const glm::vec3& coordCurr = _line[i];
            const glm::vec3& coordNext = _line[i+1];
            if (isOutsideTile(coordCurr, coordNext)) {
                Line line = Line(&_line[cut], &_line[i+1]);
                buildPolyLineSegment(line, _ctx);
                cut = i + 1;
            }
        }

        Line line = Line(&_line[cut], &_line[_line.size()]);
        buildPolyLineSegment(line, _ctx);

    }

}

void Builders::buildQuadAtPoint(const glm::vec2& _screenPosition, const glm::vec2& _size, const glm::vec2& _uvBL, const glm::vec2& _uvTR, SpriteBuilder& _ctx) {
    float halfWidth = _size.x * .5f;
    float halfHeight = _size.y * .5f;

    _ctx.addVertex(glm::vec2(-halfWidth, -halfHeight), _screenPosition, {_uvBL.x, _uvBL.y});
    _ctx.addVertex(glm::vec2(-halfWidth, halfHeight), _screenPosition, {_uvBL.x, _uvTR.y});
    _ctx.addVertex(glm::vec2(halfWidth, -halfHeight), _screenPosition, {_uvTR.x, _uvBL.y});
    _ctx.addVertex(glm::vec2(halfWidth, halfHeight), _screenPosition, {_uvTR.x, _uvTR.y});

    _ctx.indices.push_back(_ctx.numVerts + 2);
    _ctx.indices.push_back(_ctx.numVerts + 0);
    _ctx.indices.push_back(_ctx.numVerts + 1);
    _ctx.indices.push_back(_ctx.numVerts + 1);
    _ctx.indices.push_back(_ctx.numVerts + 3);
    _ctx.indices.push_back(_ctx.numVerts + 2);

    _ctx.numVerts += 4;

}

}
