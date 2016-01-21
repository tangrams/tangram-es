#pragma once

#include "data/tileData.h"

#include "glm/glm.hpp"
#include "glm/gtx/rotate_vector.hpp"

#include <vector>

namespace Tangram {


template<typename Context>
struct LineBuilder {
    // std::vector<Vertex> vertices;
    std::vector<uint16_t> indices;
    Context ctx;

    size_t numVertices = 0;
    // CapTypes cap;
    // JoinTypes join;

    void clear() {
        numVertices = 0;
        indices.clear();
    }
    void buildPolyLine(const Line& _line, int cornersOnCap, int trianglesOnJoin);
    // void addQuad(int _nPairs, int _nVertices);

    void addFan(const glm::vec3& _pC,
                const glm::vec2& _nA, const glm::vec2& _nB, const glm::vec2& _nC,
                const glm::vec2& _uA, const glm::vec2& _uB, const glm::vec2& _uC,
                int _numTriangles);

    void addCap(const glm::vec3& _coord, const glm::vec2& _normal, int _numCorners, bool _isBeginning);

    void add(const glm::vec3& coord,
                   const glm::vec2& enormal,
                   const glm::vec2& uv,
             Context& ctx) {
        numVertices++;
        addVertex(coord, enormal, uv, ctx);
    }

    // template<typename Vertex>
    void addVertex(const glm::vec3& coord,
             const glm::vec2& enormal,
             const glm::vec2& uv,
             Context& ctx);

};


namespace {
// Get 2D perpendicular of two points
glm::vec2 perp2d(const glm::vec3& _v1, const glm::vec3& _v2 ){
    return glm::vec2(_v2.y - _v1.y, _v1.x - _v2.x);
}
}
// Helper function for polyline tesselation
// inline void addPolyLineVertex(const glm::vec3& _coord, const glm::vec2& _normal, const glm::vec2& _uv) {
//     _ctx.numVertices++;
//     _ctx.addVertex(_coord, _normal, _uv);
// }

// Helper function for polyline tesselation; adds indices for pairs of vertices arranged like a line strip
void addQuad( int _nPairs, int _nVertices, std::vector<uint16_t>& indices) {
    for (int i = 0; i < _nPairs; i++) {
        indices.push_back(_nVertices - 2*i - 4);
        indices.push_back(_nVertices - 2*i - 2);
        indices.push_back(_nVertices - 2*i - 3);

        indices.push_back(_nVertices - 2*i - 3);
        indices.push_back(_nVertices - 2*i - 2);
        indices.push_back(_nVertices - 2*i - 1);
    }
}

//  Tessalate a fan geometry between points A       B
//  using their normals from a center        \ . . /
//  and interpolating their UVs               \ p /
//                                             \./
//                                              C
template<typename T>
void LineBuilder<T>::addFan(const glm::vec3& _pC,
                            const glm::vec2& _nA, const glm::vec2& _nB, const glm::vec2& _nC,
                            const glm::vec2& _uA, const glm::vec2& _uB, const glm::vec2& _uC,
                            int _numTriangles) {

    // Find angle difference
    float cross = _nA.x * _nB.y - _nA.y * _nB.x; // z component of cross(_CA, _CB)
    float angle = atan2f(cross, glm::dot(_nA, _nB));

    int startIndex = numVertices;

    // Add center vertex
    add(_pC, _nC, _uC, ctx);

    // Add vertex for point A
    add(_pC, _nA, _uA, ctx);

    // Add radial vertices
    glm::vec2 radial = _nA;
    for (int i = 0; i < _numTriangles; i++) {
        float frac = (i + 1)/(float)_numTriangles;
        radial = glm::rotate(_nA, angle * frac);
        glm::vec2 uv = (1.f - frac) * _uA + frac * _uB;
        add(_pC, radial, uv, ctx);

        // Add indices
        indices.push_back(startIndex); // center vertex
        indices.push_back(startIndex + i + (angle > 0 ? 1 : 2));
        indices.push_back(startIndex + i + (angle > 0 ? 2 : 1));
    }

}

// Function to add the vertices for line caps
template<typename T>
void LineBuilder<T>::addCap(const glm::vec3& _coord, const glm::vec2& _normal,
                            int _numCorners, bool _isBeginning) {

    float v = _isBeginning ? 0.f : 1.f; // length-wise tex coord

    if (_numCorners < 1) {
        // "Butt" cap needs no extra vertices
        return;
    } else if (_numCorners == 2) {
        // "Square" cap needs two extra vertices
        glm::vec2 tangent(-_normal.y, _normal.x);
        add(_coord, _normal + tangent, {0.f, v}, ctx);
        add(_coord, -_normal + tangent, {0.f, v}, ctx);
         // At the beginning of a line we can't form triangles with previous vertices
        if (!_isBeginning) {
            addQuad(1, numVertices, indices);
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
    addFan(_coord, nA, nB, nC, uA, uB, uC, _numCorners);
}

template<typename T>
void LineBuilder<T>::buildPolyLine(const Line& _line, int cornersOnCap, int trianglesOnJoin) {

    int lineSize = (int)_line.size();
    if (lineSize < 2) { return; }

    glm::vec3 coordPrev(_line[0]), coordCurr(_line[0]), coordNext(_line[1]);
    glm::vec2 normPrev, normNext, miterVec;

    // int cornersOnCap = (int)_ctx.cap;
    // int trianglesOnJoin = (int)_ctx.join;

    // Calculate number of used vertices to reserve enough space
    size_t nIndices = 0; //indices.size();
    nIndices += (lineSize - 1) * 6;
    size_t nVertices = 0; //numVertices;
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
    // _ctx.indices.reserve(nIndices);
    // _ctx.sizeHint(nVertices);

    // Process first point in line with an end cap
    normNext = glm::normalize(perp2d(coordCurr, coordNext));
    addCap(coordCurr, normNext, cornersOnCap, true);
    add(coordCurr, normNext, {1.0f, 0.0f}, ctx); // right corner
    add(coordCurr, -normNext, {0.0f, 0.0f}, ctx); // left corner

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

            add(coordCurr, miterVec, {1.0, v}, ctx); // right corner
            add(coordCurr, -miterVec, {0.0, v}, ctx); // left corner
            addQuad(1, numVertices, indices);

        } else {
            // Join type is a fan of triangles

            // z component of cross(normNext, normPrev)
            bool isRightTurn = (normNext.x * normPrev.y - normNext.y * normPrev.x) > 0;

            if (isRightTurn) {

                add(coordCurr, miterVec, {1.0f, v}, ctx); // right (inner) corner
                add(coordCurr, -normPrev, {0.0f, v}, ctx); // left (outer) corner
                addQuad(1, numVertices, indices);

                addFan(coordCurr, -normPrev, -normNext, miterVec,
                       {0.f, v}, {0.f, v}, {1.f, v}, trianglesOnJoin);

                add(coordCurr, miterVec, {1.0f, v}, ctx); // right (inner) corner
                add(coordCurr, -normNext, {0.0f, v}, ctx); // left (outer) corner

            } else {

                add(coordCurr, normPrev, {1.0f, v}, ctx); // right (outer) corner
                add(coordCurr, -miterVec, {0.0f, v}, ctx); // left (inner) corner
                addQuad(1, numVertices, indices);

                addFan(coordCurr, normPrev, normNext, -miterVec,
                       {1.f, v}, {1.f, v}, {0.0f, v}, trianglesOnJoin);

                add(coordCurr, normNext, {1.0f, v}, ctx); // right (outer) corner
                add(coordCurr, -miterVec, {0.0f, v}, ctx); // left (inner) corner
            }
        }
    }

    // Process last point in line with a cap
    add(coordNext, normNext, {1.f, 1.f}, ctx); // right corner
    add(coordNext, -normNext, {0.f, 1.f}, ctx); // left corner
    addQuad(1, numVertices, indices);
    addCap(coordNext, normNext, cornersOnCap , false);

#if 1
    if (nIndices != indices.size() || nVertices != numVertices) {
        LOGW("expected indices = %d => %d (%d / %d / %d)", nIndices, indices.size(), lineSize,
             cornersOnCap, trianglesOnJoin);
        LOGW("expected vertices = %d => %d (%d / %d / %d)", nVertices, numVertices, lineSize,
             cornersOnCap, trianglesOnJoin);
    }
#endif
}

}
