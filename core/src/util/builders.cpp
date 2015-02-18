#include "builders.h"

#include "tesselator.h"
#include "rectangle.h"
#include "geom.h"

#include <memory>

static auto& NO_TEXCOORDS = *(new std::vector<glm::vec2>); // denotes that texture coordinates should not be used
static auto& NO_SCALING_VECS = *(new std::vector<glm::vec2>); // denotes that scaling vectors should not be used

using Builders::CapTypes;
using Builders::JoinTypes;
using Builders::PolyLineStyle;

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

void Builders::buildPolygon(const Polygon& _polygon, std::vector<glm::vec3>& _pointsOut, std::vector<glm::vec3>& _normalOut, std::vector<int>& _indicesOut) {
    
    buildPolygon(_polygon, _pointsOut, _normalOut, _indicesOut, NO_TEXCOORDS);
    
}

void Builders::buildPolygon(const Polygon& _polygon, std::vector<glm::vec3>& _pointsOut, std::vector<glm::vec3>& _normalOut, std::vector<int>& _indicesOut, std::vector<glm::vec2>& _texcoordOut) {
    
    TESStesselator* tesselator = tessNewTess(&allocator);
    
    bool useTexCoords = &_texcoordOut != &NO_TEXCOORDS;
    
    // get the number of vertices already added
    int vertexDataOffset = (int)_pointsOut.size();
    
    Rectangle bBox;
    
    if (useTexCoords) {
        // initialize the axis-aligned bounding box of the polygon
        if(_polygon.size() > 0) {
            if(_polygon[0].size() > 0) {
                bBox.set(_polygon[0][0].x, _polygon[0][0].y, 0, 0);
            }
        }
    }
    
    // add polygon contour for every ring
    for (auto& line : _polygon) {
        if (useTexCoords) {
            bBox.growToInclude(line);
        }
        tessAddContour(tesselator, 3, line.data(), sizeof(Point), (int)line.size());
    }
    
    // call the tesselator
    glm::vec3 normal(0.0, 0.0, 1.0);
    
    if( tessTesselate(tesselator, TessWindingRule::TESS_WINDING_NONZERO, TessElementType::TESS_POLYGONS, 3, 3, &normal[0]) ) {
        
        const int numElements = tessGetElementCount(tesselator);
        const TESSindex* tessElements = tessGetElements(tesselator);
        _indicesOut.reserve(_indicesOut.size() + numElements * 3); // Pre-allocate index vector
        for(int i = 0; i < numElements; i++) {
            const TESSindex* tessElement = &tessElements[i * 3];
            for(int j = 0; j < 3; j++) {
                _indicesOut.push_back(tessElement[j] + vertexDataOffset);
            }
        }
        
        const int numVertices = tessGetVertexCount(tesselator);
        const float* tessVertices = tessGetVertices(tesselator);
        _pointsOut.reserve(_pointsOut.size() + numVertices); // Pre-allocate vertex vector
        _normalOut.reserve(_normalOut.size() + numVertices); // Pre-allocate normal vector
        if (useTexCoords) {
            _texcoordOut.reserve(_texcoordOut.size() + numVertices); // Pre-allocate texcoord vector
        }
        for(int i = 0; i < numVertices; i++) {
            if (useTexCoords) {
                float u = mapValue(tessVertices[3*i], bBox.getMinX(), bBox.getMaxX(), 0., 1.);
                float v = mapValue(tessVertices[3*i+1], bBox.getMinY(), bBox.getMaxY(), 0., 1.);
                _texcoordOut.push_back(glm::vec2(u, v));
            }
            _pointsOut.push_back(glm::vec3(tessVertices[3*i], tessVertices[3*i+1], tessVertices[3*i+2]));
            _normalOut.push_back(normal);
        }
    }
    else {
        logMsg("Tesselator cannot tesselate!!\n");
    }
    
    tessDeleteTess(tesselator);
}

void Builders::buildPolygonExtrusion(const Polygon& _polygon, const float& _minHeight, std::vector<glm::vec3>& _pointsOut, std::vector<glm::vec3>& _normalOut, std::vector<int>& _indicesOut) {
    
    buildPolygonExtrusion(_polygon, _minHeight, _pointsOut, _normalOut, _indicesOut, NO_TEXCOORDS);
    
}

void Builders::buildPolygonExtrusion(const Polygon& _polygon, const float& _minHeight, std::vector<glm::vec3>& _pointsOut, std::vector<glm::vec3>& _normalOut, std::vector<int>& _indicesOut, std::vector<glm::vec2>& _texcoordOut) {
    
    int vertexDataOffset = (int)_pointsOut.size();
    
    glm::vec3 upVector(0.0f, 0.0f, 1.0f);
    glm::vec3 normalVector;
    
    bool useTexCoords = &_texcoordOut != &NO_TEXCOORDS;
    
    for (auto& line : _polygon) {
        
        size_t lineSize = line.size();
        _pointsOut.reserve(_pointsOut.size() + lineSize * 4); // Pre-allocate vertex vector
        _normalOut.reserve(_normalOut.size() + lineSize * 4); // Pre-allocate normal vector
        _indicesOut.reserve(_indicesOut.size() + lineSize * 6); // Pre-allocate index vector
        if (useTexCoords) {
            _texcoordOut.reserve(_texcoordOut.size() + lineSize * 4); // Pre-allocate texcoord vector
        }
        
        for (size_t i = 0; i < lineSize - 1; i++) {
            
            normalVector = glm::cross(upVector, (line[i+1] - line[i]));
            normalVector = glm::normalize(normalVector);
            
            // 1st vertex top
            _pointsOut.push_back(line[i]);
            _normalOut.push_back(normalVector);
            
            // 2nd vertex top
            _pointsOut.push_back(line[i+1]);
            _normalOut.push_back(normalVector);
            
            // 1st vertex bottom
            _pointsOut.push_back(glm::vec3(line[i].x, line[i].y, _minHeight));
            _normalOut.push_back(normalVector);
            
            // 2nd vertex bottom
            _pointsOut.push_back(glm::vec3(line[i+1].x, line[i+1].y, _minHeight));
            _normalOut.push_back(normalVector);
            
            //Start the index from the previous state of the vertex Data
            _indicesOut.push_back(vertexDataOffset);
            _indicesOut.push_back(vertexDataOffset + 1);
            _indicesOut.push_back(vertexDataOffset + 2);
            
            _indicesOut.push_back(vertexDataOffset + 1);
            _indicesOut.push_back(vertexDataOffset + 3);
            _indicesOut.push_back(vertexDataOffset + 2);
            
            if (useTexCoords) {
                _texcoordOut.push_back(glm::vec2(1.,0.));
                _texcoordOut.push_back(glm::vec2(0.,0.));
                _texcoordOut.push_back(glm::vec2(1.,1.));
                _texcoordOut.push_back(glm::vec2(0.,1.));
            }
            
            vertexDataOffset += 4;
            
        }
    }
}

// Get 2D perpendicular of two points
glm::vec2 perp2d(const glm::vec3& _v1, const glm::vec3& _v2 ){
    return glm::vec2(_v2.y - _v1.y, _v1.x - _v2.x);
}

// Get 2D vector rotated 
glm::vec2 rotate (const glm::vec2& _v, float _radians) {
    float cos = std::cos(_radians);
    float sin = std::sin(_radians);
    return glm::vec2(_v.x * cos - _v.y * sin, _v.x * sin + _v.y * cos);
}

// Add a single vertex according the information it have and need
void addVertex(const glm::vec3& _coord,
               const glm::vec2& _normal,
               const glm::vec2& _uv,
               float _halfWidth,
               std::vector<glm::vec3>& _pointsOut,
               std::vector<glm::vec2>& _scalingVecsOut,
               std::vector<glm::vec2>& _texCoordOut) {

    if (&_scalingVecsOut != &NO_SCALING_VECS) {
        _pointsOut.push_back(_coord);
        _scalingVecsOut.push_back(_normal);
    } else {
        _pointsOut.push_back(glm::vec3( _coord.x + _normal.x * _halfWidth, _coord.y + _normal.y * _halfWidth, _coord.z));
    }

    if(&_texCoordOut != &NO_TEXCOORDS){
         _texCoordOut.push_back(_uv);
    }
}

//  Add to equidistant pairs of vertices to then be indexed in LINE_STRIP indexes
void addVertexPair (const glm::vec3& _coord,
                    const glm::vec2& _normal,
                    float _pct,
                    float _halfWidth,
                    std::vector<glm::vec3>& _pointsOut,
                    std::vector<glm::vec2>& _scalingVecsOut,
                    std::vector<glm::vec2>& _texCoordOut){

    addVertex(_coord, _normal, glm::vec2(1.0,_pct*1.0), _halfWidth, _pointsOut, _scalingVecsOut, _texCoordOut);
    addVertex(_coord, -_normal, glm::vec2(0.0,_pct*1.0), _halfWidth, _pointsOut, _scalingVecsOut, _texCoordOut);
}

// Index pairs of vertices to make triangles based like LINE_STRIP
void indexPairs ( int _nPairs, int _vertexDataOffset, std::vector<int>& _indicesOut) {
    for (int i = 0; i < _nPairs; i++) {
        _indicesOut.push_back(_vertexDataOffset + 2*i+2);
        _indicesOut.push_back(_vertexDataOffset + 2*i+1);
        _indicesOut.push_back(_vertexDataOffset + 2*i);
        
        _indicesOut.push_back(_vertexDataOffset + 2*i+2);
        _indicesOut.push_back(_vertexDataOffset + 2*i+3);
        _indicesOut.push_back(_vertexDataOffset + 2*i+1);
    }
}

//  Tessalate a FAN geometry between points A       B
//  using their normals from a center        \ . . /
//  and interpolating their UVs               \ p /
//                                             \./
//                                              C
void addFan (const glm::vec3& _coord, 
             const glm::vec2& _nA, const glm::vec2& _nC, const glm::vec2& _nB, 
             const glm::vec2& _uA, const glm::vec2& _uC, const glm::vec2& _uB, 
             bool _isSigned, int _numTriangles, float _halfWidth,
             std::vector<glm::vec3>& _pointsOut,
             std::vector<glm::vec2>& _scalingVecsOut,
             std::vector<int>& _indicesOut,
             std::vector<glm::vec2>& _texCoordOut) {

    int vertexDataOffset = _pointsOut.size();

    if (_numTriangles < 1) {
        return;
    }

    glm::vec2 normCurr = _nA;
    glm::vec2 normPrev = glm::vec2(0.0,0.0);

    float angle_delta = glm::dot(_nA, _nB);
    if (angle_delta < -1.0) {
        angle_delta = -1.0;
    } else if (angle_delta > 1.0){
        angle_delta = 1.0;
    }
    angle_delta = std::acos(angle_delta)/(float)_numTriangles;

    if (!_isSigned) {
        angle_delta *= -1;
    }

    glm::vec2 uvCurr = _uA;
    glm::vec2 uv_delta = (_uB-_uA)/(float)_numTriangles;

    //  Add the first and CENTER vertex 
    //  The triangles will be composed on FAN style arround it
    addVertex(_coord, _nC, _uC, _halfWidth, _pointsOut, _scalingVecsOut, _texCoordOut);

    //  Add first corner
    addVertex(_coord, normCurr, _uA, _halfWidth, _pointsOut, _scalingVecsOut, _texCoordOut);

    // Iterate through the rest of the coorners
    for (int i = 0; i < _numTriangles; i++) {
        normPrev = glm::normalize(normCurr);
        normCurr = rotate( glm::normalize(normCurr), angle_delta);     //  Rotate the extrusion normal

        if (_numTriangles == 4 && (i == 0 || i == _numTriangles - 2)) {
            float scale = 2.0 / (1.0 + std::fabs(glm::dot(normPrev, normCurr)));
            normCurr *= scale*scale;
        }

        uvCurr = uvCurr+uv_delta;

        addVertex(_coord, normCurr, uvCurr, _halfWidth, _pointsOut, _scalingVecsOut, _texCoordOut);      //  Add computed corner
    }

    for (int i = 0; i < _numTriangles; i++) {
        if (_isSigned) {
            _indicesOut.push_back(vertexDataOffset + i+2);
            _indicesOut.push_back(vertexDataOffset );
            _indicesOut.push_back(vertexDataOffset + i+1);
        } else {
            _indicesOut.push_back(vertexDataOffset + i+1);
            _indicesOut.push_back(vertexDataOffset );
            _indicesOut.push_back(vertexDataOffset + i+2);
        }
    }
}

//  Function to add the vertex need for line caps,
//  because re-use the buffers needs to be at the end
void addCap (const glm::vec3& _coord,
             const glm::vec2& _normal,
             int _numCorners, bool _isBeginning, float _halfWidth,
             std::vector<glm::vec3>& _pointsOut,
             std::vector<glm::vec2>& _scalingVecsOut,
             std::vector<int>& _indicesOut,
             std::vector<glm::vec2>& _texCoordOut ) {

    if (_numCorners < 1) {
        return;
    }

    // UVs
    glm::vec2 uvA = glm::vec2(0.0,0.0); // Begining angle UVs
    glm::vec2 uvC = glm::vec2(0.5,0.0); // center point UVs
    glm::vec2 uvB = glm::vec2(1.0,0.0); // Ending angle UVs
 
    if (!_isBeginning) {
        uvA = glm::vec2(0.0,1.0);       // Begining angle UVs
        uvC = glm::vec2(0.5,1.0);       // center point UVs
        uvB = glm::vec2(1.0,1.0);       // Ending angle UVs
    }

    addFan( _coord, 
            -_normal, glm::vec2(0,0), _normal, 
            uvA, uvC, uvB, 
            _isBeginning, _numCorners*2, 
            _halfWidth, _pointsOut, _scalingVecsOut, _indicesOut, _texCoordOut);
}

//  Cross product
float signed_area (const glm::vec3& _v1, const glm::vec3& _v2, const glm::vec3& _v3) {
    return (_v2.x-_v1.x)*(_v3.y-_v1.y) - (_v3.x-_v1.x)*(_v2.y-_v1.y);
};

/* TODO:
 *      - this is taken from WebGL needs a better adaptation
 *      - Take a look to https://github.com/tangrams/tangram/blob/master/src/gl/gl_builders.js#L581
 */ 
float valuesWithinTolerance ( float _a, float _b, float _tolerance = 0.001) {
    return std::abs(_a - _b) < _tolerance;
}

// Tests if a line segment (from point A to B) is nearly coincident with the edge of a tile
bool isOnTileEdge (const glm::vec3& _pa, const glm::vec3& _pb) {
    float tolerance = 0.0002; // tweak this adjust if catching too few/many line segments near tile edges
                                            // TODO: make tolerance configurable by source if necessary
    glm::vec2 tile_min = glm::vec2(-1.0,-1.0);
    glm::vec2 tile_max = glm::vec2(1.0,1.0);

    // std::string edge = "";
    if (valuesWithinTolerance(_pa.x, tile_min.x, tolerance) && valuesWithinTolerance(_pb.x, tile_min.x, tolerance)) {
        // edge = 'left';
        return true;
    }
    else if (valuesWithinTolerance(_pa.x, tile_max.x, tolerance) && valuesWithinTolerance(_pb.x, tile_max.x, tolerance)) {
        // edge = 'right';
        return true;
    }
    else if (valuesWithinTolerance(_pa.y, tile_min.y, tolerance) && valuesWithinTolerance(_pb.y, tile_min.y, tolerance)) {
        // edge = 'top';
        return true;
    }
    else if (valuesWithinTolerance(_pa.y, tile_max.y, tolerance) && valuesWithinTolerance(_pb.y, tile_max.y, tolerance)) {
        // edge = 'bottom';
        return true;
    }
    // return edge;
    return false;
}

void buildGeneralPolyLine(const Line& _line,
                          std::vector<glm::vec3>& _pointsOut,
                          std::vector<glm::vec2>& _scalingVecsOut,
                          std::vector<int>& _indicesOut,
                          std::vector<glm::vec2>& _texCoordOut,
                          const PolyLineStyle& _style) {

    // TODO:
    //      This flags have to be pass from the style:
    //    
    int lineSize = (int)_line.size();
    
    if (lineSize < 2) {
        return;
    }
    
    // TODO:
    //      - pre-allocate vectors?? how?
    //      - Solution: 
    //                  1. calculate the worst scenerio
    //
    // _pointsOut.reserve(_pointsOut.size() + lineSize * 2); // Pre-allocate vertex vector
    // _indicesOut.reserve(_indicesOut.size() + (lineSize - 1) * 6); // Pre-allocate index vector
    // if (useTexCoords) {
    //     _texCoordOut.reserve(_texCoordOut.size() + lineSize * 2); // Pre-allocate texcoords vector
    // }
    // if (useScalingVecs) {
    //     _scalingVecsOut.reserve(_scalingVecsOut.size() + lineSize * 2); // Pre-allocate scalingvec vector
    // }
    
    glm::vec3 coordPrev, coordCurr, coordNext;
    glm::vec2 normPrev, normCurr, normNext;

    int cornersOnCap = (int)_style.cap;
    int trianglesOnJoin = (int)_style.join;

    int vertexDataOffset = (int)_pointsOut.size();
    int nPairs = 0;
    
    bool hasPrev = false;
    bool hasNext = false;
    
    for (int i = 0; i < lineSize; i++) {

        // There is a next one?
        hasNext = i+1 < lineSize;

        if (hasPrev) {
            // If there is a previous one, copy the current (previous) values on *Prev values
            coordPrev = coordCurr;
            normPrev = glm::normalize(perp2d(coordPrev, _line[i]) );
        } else if (i == 0 && _style.closePolygon) {
            // If is the first point and is a close polygon
            // TODO   
            bool needToClose = true;
            if (_style.removeTileEdges) {
                if(isOnTileEdge(_line[i], _line[lineSize-2])) {
                    needToClose = false;
                }
            }

            if (needToClose) {
                coordPrev = _line[lineSize-2];
                normPrev = glm::normalize(perp2d(coordPrev, _line[i]));
                hasPrev = true;
            }
        }

        // Assign current coordinate
        coordCurr = _line[i];

        if (hasNext) {
            coordNext = _line[i+1];
        } else if (_style.closePolygon) {
            // If is the last point a close polygon
            coordNext = _line[1];
            hasNext = true;
        }

        if (hasNext) {
            // If is not the last one get next coordinates and calculate the right normal

            normNext = glm::normalize(perp2d(coordCurr, coordNext));
            if (_style.removeTileEdges) {
                if (isOnTileEdge(coordCurr, coordNext) ) {
                    normCurr = glm::normalize(perp2d(coordPrev, coordCurr));
                    
                    if (hasPrev) {
                        addVertexPair(coordCurr, normCurr, (float)i/(float)lineSize, _style.halfWidth, _pointsOut, _scalingVecsOut, _texCoordOut);

                        // Add vertices to buffer acording their index
                        indexPairs(nPairs, vertexDataOffset, _indicesOut);
                        
                        vertexDataOffset = (int)_pointsOut.size();
                        nPairs = 0;
                    }
                    hasPrev = false;
                    continue;
                }
            }
        }

        //  Compute current normal
        if (hasPrev) {
            //  If there is a PREVIUS ...
            if (hasNext) {
                // ... and a NEXT ONE, compute previus and next normals (scaled by the angle with the last prev)
                normCurr = glm::normalize(normPrev + normNext);
                float scale = 2.0f / (1.0f + std::fabs(glm::dot(normPrev, normCurr)));
                normCurr *= scale*scale;
            } else {
                // ... and there is NOT a NEXT ONE, copy the previus next one (which is the current one)
                normCurr = glm::normalize(perp2d(coordPrev, coordCurr));
            }
        } else {
            // If is NOT a PREVIUS ...
            if (hasNext) {
                // ... and a NEXT ONE,
                normNext = glm::normalize(perp2d(coordCurr, coordNext));
                normCurr = normNext;
            } else {
                // ... and NOT a NEXT ONE, nothing to do (without prev or next one this is just a point)
                continue;
            }
        }

        if (hasPrev || hasNext) {
            
            // If is the BEGINING of a LINE
            if (i == 0 && !hasPrev && !_style.closePolygon) {
                // Add previus vertices to buffer and reset the index pairs counter
                // Because we are going to add more triangles.
                indexPairs(nPairs, vertexDataOffset, _indicesOut);
                
                addCap( coordCurr, normCurr,
                        cornersOnCap, true, 
                        _style.halfWidth, _pointsOut, _scalingVecsOut, _indicesOut, _texCoordOut);

                vertexDataOffset = (int)_pointsOut.size();
                nPairs = 0;
            }

            // If is a JOIN
            if(trianglesOnJoin != 0 ) {
                // Add previus vertices to buffer and reset the index pairs counter
                // Because we are going to add more triangles.

                bool isSigned = signed_area(coordPrev, coordCurr, coordNext) > 0;

                glm::vec2 nA = normPrev;     // normal to point A (aT)
                glm::vec2 nC = -normCurr;    // normal to center (-vP)
                glm::vec2 nB = normNext;     // normal to point B (bT)

                float pct = (float)i/(float)lineSize;

                glm::vec2 uA = glm::vec2(1.0,pct);
                glm::vec2 uC = glm::vec2(0.0,pct);
                glm::vec2 uB = glm::vec2(1.0,pct);
                
                if (isSigned) {
                    addVertex(coordCurr, nA, uA, _style.halfWidth, _pointsOut, _scalingVecsOut, _texCoordOut);
                    addVertex(coordCurr, nC, uC, _style.halfWidth, _pointsOut, _scalingVecsOut, _texCoordOut);
                } else {
                    nA = -normPrev;
                    nC = normCurr;
                    nB = -normNext;
                    uA = glm::vec2(0.0,pct);
                    uC = glm::vec2(1.0,pct);
                    uB = glm::vec2(0.0,pct);
                    addVertex(coordCurr, nC, uC, _style.halfWidth, _pointsOut, _scalingVecsOut, _texCoordOut);
                    addVertex(coordCurr, nA, uA, _style.halfWidth, _pointsOut, _scalingVecsOut, _texCoordOut);
                }
            
                indexPairs(nPairs, vertexDataOffset, _indicesOut);

                addFan( coordCurr, nA, nC, nB, uA, uC, uB, isSigned, trianglesOnJoin, 
                       _style.halfWidth, _pointsOut, _scalingVecsOut, _indicesOut, _texCoordOut);

                vertexDataOffset = (int)_pointsOut.size();
                nPairs = 0;

                if (isSigned) {
                    addVertex(coordCurr, nB, uB, _style.halfWidth, _pointsOut, _scalingVecsOut, _texCoordOut);
                    addVertex(coordCurr, nC, uC, _style.halfWidth, _pointsOut, _scalingVecsOut, _texCoordOut);
                } else {
                    addVertex(coordCurr, nC, uC, _style.halfWidth, _pointsOut, _scalingVecsOut, _texCoordOut);
                    addVertex(coordCurr, nB, uB, _style.halfWidth, _pointsOut, _scalingVecsOut, _texCoordOut);
                }
                
            } else {
                addVertexPair(  coordCurr, normCurr, 
                                (float)i/((float)lineSize-1.0), 
                                _style.halfWidth, _pointsOut, _scalingVecsOut, _texCoordOut);
            }
            
            if (i+1 < lineSize) {
               nPairs++;
            }

            hasPrev = true;
        }
    }
    
    // Add vertices to buffer acording their index
    indexPairs(nPairs, vertexDataOffset, _indicesOut);
    

    // If is the END OF a LINE
    if(!_style.closePolygon) {
        vertexDataOffset = (int)_pointsOut.size();
        addCap(coordCurr, normCurr, cornersOnCap , false, _style.halfWidth, _pointsOut, _scalingVecsOut, _indicesOut, _texCoordOut);
    }
}

void Builders::buildPolyLine(const Line& _line, std::vector<glm::vec3>& _pointsOut, std::vector<int>& _indicesOut, const PolyLineStyle& _style) {

    buildGeneralPolyLine(_line, _pointsOut, NO_SCALING_VECS, _indicesOut, NO_TEXCOORDS, _style);
    
}

void Builders::buildPolyLine(const Line& _line, std::vector<glm::vec3>& _pointsOut, std::vector<int>& _indicesOut, std::vector<glm::vec2>& _texcoordOut, const PolyLineStyle& _style) {
    
    buildGeneralPolyLine(_line, _pointsOut, NO_SCALING_VECS, _indicesOut, _texcoordOut, _style);
    
}

void Builders::buildScalablePolyLine(const Line& _line, std::vector<glm::vec3>& _pointsOut, std::vector<glm::vec2>& _scalingVecsOut, std::vector<int>& _indicesOut, const PolyLineStyle& _style) {
    
    buildGeneralPolyLine(_line, _pointsOut, _scalingVecsOut, _indicesOut, NO_TEXCOORDS, _style);
    
}

void Builders::buildScalablePolyLine(const Line& _line, std::vector<glm::vec3>& _pointsOut, std::vector<glm::vec2>& _scalingVecsOut, std::vector<int>& _indicesOut, std::vector<glm::vec2>& _texcoordOut, const PolyLineStyle& _style) {
    
    buildGeneralPolyLine(_line, _pointsOut, _scalingVecsOut, _indicesOut, _texcoordOut, _style);
}

void Builders::buildQuadAtPoint(const Point& _point, const glm::vec3& _normal, float halfWidth, float height, std::vector<glm::vec3>& _pointsOut, std::vector<int>& _indicesOut) {

}
