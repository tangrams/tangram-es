#include "builders.h"

#include "aabb.h"
#include "geom.h"
#include "glm/gtx/rotate_vector.hpp"
#include "platform.h"
#include <memory>

#if USE_LIBTESS
#include "tesselator.h"
#else
#include "earcut.hpp/include/earcut.hpp"

namespace mapbox { namespace util {
template <>
struct nth<0, Tangram::Point> {
    inline static float get(const Tangram::Point &t) {
        return t.x;
    };
};
template <>
struct nth<1, Tangram::Point> {
    inline static float get(const Tangram::Point &t) {
        return t.y;
    };
};
}}

#endif

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

#if USE_LIBTESS

void* alloc(void* _userData, unsigned int _size) {
    return malloc(_size);
}

void* realloc(void* _userData, void* _ptr, unsigned int _size) {
    return realloc(_ptr, _size);
}

void free(void* _userData, void* _ptr) {
    free(_ptr);
}

static TESSalloc allocator = {&alloc, &realloc, &free, nullptr,
                              64, // meshEdgeBucketSize
                              64, // meshVertexBucketSize
                              16,  // meshFaceBucketSize
                              64, // dictNodeBucketSize
                              16,  // regionBucketSize
                              64  // extraVertices
                             };

void Builders::buildPolygon(const Polygon& _polygon, float _height, PolygonBuilder& _ctx) {

    TESStesselator* tesselator = tessNewTess(&allocator);
    isect2d::AABB bbox;

    if (_ctx.useTexCoords && _polygon.size() > 0 && _polygon[0].size() > 0) {
        // initialize the axis-aligned bounding box of the polygon
        bbox = isect2d::AABB(_polygon[0][0].x, _polygon[0][0].y, 0, 0);
    }

    // add polygon contour for every ring
    for (auto& line : _polygon) {
        if (_ctx.useTexCoords) {
            for (auto& p : line) {
                bbox.include(p.x, p.y);
            }
        }
        tessAddContour(tesselator, 3, line.data(), sizeof(Point), (int)line.size());
    }

    // call the tesselator
    glm::vec3 normal(0.0, 0.0, 1.0);

    if (tessTesselate(tesselator, TessWindingRule::TESS_WINDING_NONZERO, TessElementType::TESS_POLYGONS, 3, 3, &normal[0])) {

        const int numElements = tessGetElementCount(tesselator);
        const TESSindex* tessElements = tessGetElements(tesselator);
        _ctx.indices.reserve(_ctx.indices.size() + numElements * 3); // Pre-allocate index vector
        for (int i = 0; i < numElements; i++) {
            const TESSindex* tessElement = &tessElements[i * 3];
            for (int j = 0; j < 3; j++) {
                _ctx.indices.push_back(tessElement[j] + _ctx.numVertices);
            }
        }

        const int numVertices = tessGetVertexCount(tesselator);
        const float* tessVertices = tessGetVertices(tesselator);

        _ctx.numVertices += numVertices;
        _ctx.sizeHint(_ctx.numVertices);

        for (int i = 0; i < numVertices; i++) {
            glm::vec3 coord(tessVertices[3*i], tessVertices[3*i+1], _height);
            glm::vec2 uv(0);

            if (_ctx.useTexCoords) {
                float u = mapValue(coord.x, bbox.m_min.x, bbox.m_max.x, 0., 1.);
                float v = mapValue(coord.y, bbox.m_min.y, bbox.m_max.y, 0., 1.);
                uv = glm::vec2(u, v);
            }
            _ctx.addVertex(coord, normal, uv);
        }
    } else {
        logMsg("Tesselator cannot tesselate!!\n");
    }

    tessDeleteTess(tesselator);
}

#else


void Builders::buildPolygon(const Polygon& _polygon, float _height, PolygonBuilder& _ctx) {

    mapbox::Earcut<float, uint16_t> earcut;

    earcut(_polygon);

    _ctx.indices = std::move(earcut.indices);

    isect2d::AABB bbox;

    if (_ctx.useTexCoords) {
        if (_polygon.size() > 0 && _polygon[0].size() > 0) {
            // initialize the axis-aligned bounding box of the polygon
            bbox = isect2d::AABB(_polygon[0][0].x, _polygon[0][0].y, 0, 0);
        }
        for (auto& line : _polygon) {
            for (auto& p : line) {
                bbox.include(p.x, p.y);
            }
        }
    }

    // call the tesselator
    glm::vec3 normal(0.0, 0.0, 1.0);

    _ctx.numVertices += earcut.vertices.size();
    _ctx.sizeHint(_ctx.numVertices);

    for (auto& p : earcut.vertices) {
        glm::vec2 uv(0);
        glm::vec3 coord(p[0], p[1], _height);

        if (_ctx.useTexCoords) {
            float u = mapValue(coord.x, bbox.m_min.x, bbox.m_max.x, 0., 1.);
            float v = mapValue(coord.y, bbox.m_min.y, bbox.m_max.y, 0., 1.);
            uv = glm::vec2(u, v);
        }
        _ctx.addVertex(coord, normal, uv);
    }
}


#endif

void Builders::buildPolygonExtrusion(const Polygon& _polygon, float _minHeight, float _maxHeight, PolygonBuilder& _ctx) {

    int vertexDataOffset = (int)_ctx.numVertices;

    glm::vec3 upVector(0.0f, 0.0f, 1.0f);
    glm::vec3 normalVector;

    for (auto& line : _polygon) {

        size_t lineSize = line.size();
        _ctx.indices.reserve(_ctx.indices.size() + lineSize * 6);

        _ctx.numVertices += (lineSize - 1) * 4;
        _ctx.sizeHint(_ctx.numVertices);

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
        glm::vec2 uv = (1.f - frac) * _uA + frac * _uB;
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

float valuesWithinTolerance(float _a, float _b, float _tolerance = 0.001) {
    return fabsf(_a - _b) < _tolerance;
}

// Tests if a line segment (from point A to B) is nearly coincident with the edge of a tile
bool isOnTileEdge(const glm::vec3& _pa, const glm::vec3& _pb) {

    float tolerance = 0.0002; // tweak this adjust if catching too few/many line segments near tile edges
    // TODO: make tolerance configurable by source if necessary
    glm::vec2 tile_min(-1.0, -1.0);
    glm::vec2 tile_max(1.0, 1.0);

    return (valuesWithinTolerance(_pa.x, tile_min.x, tolerance) && valuesWithinTolerance(_pb.x, tile_min.x, tolerance)) ||
           (valuesWithinTolerance(_pa.x, tile_max.x, tolerance) && valuesWithinTolerance(_pb.x, tile_max.x, tolerance)) ||
           (valuesWithinTolerance(_pa.y, tile_min.y, tolerance) && valuesWithinTolerance(_pb.y, tile_min.y, tolerance)) ||
           (valuesWithinTolerance(_pa.y, tile_max.y, tolerance) && valuesWithinTolerance(_pb.y, tile_max.y, tolerance));
}

void Builders::buildPolyLine(const Line& _line, PolyLineBuilder& _ctx) {

    int lineSize = (int)_line.size();
    if (lineSize < 2) { return; }

    glm::vec3 coordPrev(_line[0]), coordCurr(_line[0]), coordNext(_line[1]);
    glm::vec2 normPrev, normNext, miterVec;

    int cornersOnCap = (int)_ctx.cap;
    int trianglesOnJoin = (int)_ctx.join;

    // Calculate number of used vertices to reserve enough space
    size_t nIndices = _ctx.indices.size();
    nIndices += (lineSize - 1) * 6;
    size_t nVertices = _ctx.numVertices;
    nVertices += lineSize * 2;

    if (trianglesOnJoin > 0) {
        int nJoins = (lineSize - 2);
        nVertices += nJoins * (trianglesOnJoin + 4);
        nIndices += nJoins * (trianglesOnJoin * 3);
    }
    if (cornersOnCap == 2) {
        nVertices += 2 * 2;
        nIndices += 2 * 3;
    }
    else if (cornersOnCap > 2) {
        nVertices += 2 * (cornersOnCap + 2);
        nIndices += 2 * cornersOnCap * 3;
    }
    _ctx.indices.reserve(nIndices);
    _ctx.sizeHint(nVertices);

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
        miterVec *= fminf(scale, 5.0f); // clamps our miter vector to an arbitrary length

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

#if 1
    if (nIndices != _ctx.indices.size() || nVertices != _ctx.numVertices) {
        logMsg("expected indices = %d => %d (%d / %d / %d)\n", nIndices, _ctx.indices.size(), lineSize,
               cornersOnCap, trianglesOnJoin);
        logMsg("expected vertices = %d => %d (%d / %d / %d)\n", nVertices, _ctx.numVertices, lineSize,
               cornersOnCap, trianglesOnJoin);
    }
#endif
}

void Builders::buildOutline(const Line& _line, PolyLineBuilder& _ctx) {

    int cut = 0;

    for (size_t i = 0; i < _line.size() - 1; i++) {
        const glm::vec3& coordCurr = _line[i];
        const glm::vec3& coordNext = _line[i+1];
        if (isOnTileEdge(coordCurr, coordNext)) {
            Line line = Line(&_line[cut], &_line[i+1]);
            buildPolyLine(line, _ctx);
            cut = i + 1;
        }
    }

    Line line = Line(&_line[cut], &_line[_line.size()]);
    buildPolyLine(line, _ctx);

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
