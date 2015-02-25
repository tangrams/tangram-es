#include "builders.h"

#include "tesselator.h"
#include "rectangle.h"
#include "geom.h"

#include <memory>

using Builders::CapTypes;
using Builders::JoinTypes;
using Builders::PolyLineOptions;
using Builders::PolygonOutput;
using Builders::PolyLineOutput;
using Builders::NO_TEXCOORDS;
using Builders::NO_SCALING_VECS;

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

void Builders::buildPolygon(const Polygon& _polygon, PolygonOutput& _out) {
    
    TESStesselator* tesselator = tessNewTess(&allocator);
    
    bool useTexCoords = (&_out.texcoords != &NO_TEXCOORDS);
    
    // get the number of vertices already added
    int vertexDataOffset = (int)_out.points.size();
    
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
        _out.indices.reserve(_out.indices.size() + numElements * 3); // Pre-allocate index vector
        for(int i = 0; i < numElements; i++) {
            const TESSindex* tessElement = &tessElements[i * 3];
            for(int j = 0; j < 3; j++) {
                _out.indices.push_back(tessElement[j] + vertexDataOffset);
            }
        }
        
        const int numVertices = tessGetVertexCount(tesselator);
        const float* tessVertices = tessGetVertices(tesselator);
        _out.points.reserve(_out.points.size() + numVertices); // Pre-allocate vertex vector
        _out.normals.reserve(_out.normals.size() + numVertices); // Pre-allocate normal vector
        if (useTexCoords) {
            _out.texcoords.reserve(_out.texcoords.size() + numVertices); // Pre-allocate texcoord vector
        }
        for(int i = 0; i < numVertices; i++) {
            if (useTexCoords) {
                float u = mapValue(tessVertices[3*i], bBox.getMinX(), bBox.getMaxX(), 0., 1.);
                float v = mapValue(tessVertices[3*i+1], bBox.getMinY(), bBox.getMaxY(), 0., 1.);
                _out.texcoords.push_back(glm::vec2(u, v));
            }
            _out.points.push_back(glm::vec3(tessVertices[3*i], tessVertices[3*i+1], tessVertices[3*i+2]));
            _out.normals.push_back(normal);
        }
    }
    else {
        logMsg("Tesselator cannot tesselate!!\n");
    }
    
    tessDeleteTess(tesselator);
}

void Builders::buildPolygonExtrusion(const Polygon& _polygon, const float& _minHeight, PolygonOutput& _out) {
    
    int vertexDataOffset = (int)_out.points.size();
    
    glm::vec3 upVector(0.0f, 0.0f, 1.0f);
    glm::vec3 normalVector;
    
    bool useTexCoords = (&_out.texcoords != &NO_TEXCOORDS);
    
    for (auto& line : _polygon) {
        
        size_t lineSize = line.size();
        _out.points.reserve(_out.points.size() + lineSize * 4); // Pre-allocate vertex vector
        _out.normals.reserve(_out.normals.size() + lineSize * 4); // Pre-allocate normal vector
        _out.indices.reserve(_out.indices.size() + lineSize * 6); // Pre-allocate index vector
        if (useTexCoords) {
            _out.texcoords.reserve(_out.texcoords.size() + lineSize * 4); // Pre-allocate texcoord vector
        }
        
        for (size_t i = 0; i < lineSize - 1; i++) {
            
            normalVector = glm::cross(upVector, (line[i+1] - line[i]));
            normalVector = glm::normalize(normalVector);
            
            // 1st vertex top
            _out.points.push_back(line[i]);
            _out.normals.push_back(normalVector);
            
            // 2nd vertex top
            _out.points.push_back(line[i+1]);
            _out.normals.push_back(normalVector);
            
            // 1st vertex bottom
            _out.points.push_back(glm::vec3(line[i].x, line[i].y, _minHeight));
            _out.normals.push_back(normalVector);
            
            // 2nd vertex bottom
            _out.points.push_back(glm::vec3(line[i+1].x, line[i+1].y, _minHeight));
            _out.normals.push_back(normalVector);
            
            //Start the index from the previous state of the vertex Data
            _out.indices.push_back(vertexDataOffset);
            _out.indices.push_back(vertexDataOffset + 1);
            _out.indices.push_back(vertexDataOffset + 2);
            
            _out.indices.push_back(vertexDataOffset + 1);
            _out.indices.push_back(vertexDataOffset + 3);
            _out.indices.push_back(vertexDataOffset + 2);
            
            if (useTexCoords) {
                _out.texcoords.push_back(glm::vec2(1.,0.));
                _out.texcoords.push_back(glm::vec2(0.,0.));
                _out.texcoords.push_back(glm::vec2(1.,1.));
                _out.texcoords.push_back(glm::vec2(0.,1.));
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

// Helper function for polyline tesselation
void addPolyLineVertex(const glm::vec3& _coord, const glm::vec2& _normal, const glm::vec2& _uv, float _halfWidth, PolyLineOutput _out) {

    if (&_out.scalingVecs != &NO_SCALING_VECS) {
        _out.points.push_back(_coord);
        _out.scalingVecs.push_back(_normal);
    } else {
        _out.points.push_back(glm::vec3( _coord.x + _normal.x * _halfWidth, _coord.y + _normal.y * _halfWidth, _coord.z));
    }

    if(&_out.texcoords != &NO_TEXCOORDS){
         _out.texcoords.push_back(_uv);
    }
}

// Helper function for polyline tesselation; adds indices for pairs of vertices arranged like a line strip
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
             PolyLineOutput _out) {

    int vertexDataOffset = _out.points.size();

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
    addPolyLineVertex(_coord, _nC, _uC, _halfWidth, _out);

    //  Add first corner
    addPolyLineVertex(_coord, normCurr, _uA, _halfWidth, _out);

    // Iterate through the rest of the coorners
    for (int i = 0; i < _numTriangles; i++) {
        normPrev = glm::normalize(normCurr);
        normCurr = rotate( glm::normalize(normCurr), angle_delta);     //  Rotate the extrusion normal

        if (_numTriangles == 4 && (i == 0 || i == _numTriangles - 2)) {
            float scale = 2.0 / (1.0 + std::fabs(glm::dot(normPrev, normCurr)));
            normCurr *= scale*scale;
        }

        uvCurr = uvCurr+uv_delta;

        addPolyLineVertex(_coord, normCurr, uvCurr, _halfWidth, _out);      //  Add computed corner
    }

    for (int i = 0; i < _numTriangles; i++) {
        if (_isSigned) {
            _out.indices.push_back(vertexDataOffset + i+2);
            _out.indices.push_back(vertexDataOffset );
            _out.indices.push_back(vertexDataOffset + i+1);
        } else {
            _out.indices.push_back(vertexDataOffset + i+1);
            _out.indices.push_back(vertexDataOffset );
            _out.indices.push_back(vertexDataOffset + i+2);
        }
    }
}

//  Function to add the vertex need for line caps,
//  because re-use the buffers needs to be at the end
void addCap (const glm::vec3& _coord, const glm::vec2& _normal, int _numCorners, bool _isBeginning, float _halfWidth, PolyLineOutput _out) {

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
            _halfWidth, _out);
}

//  Cross product
float signed_area (const glm::vec3& _v1, const glm::vec3& _v2, const glm::vec3& _v3) {
    return (_v2.x-_v1.x)*(_v3.y-_v1.y) - (_v3.x-_v1.x)*(_v2.y-_v1.y);
};

float valuesWithinTolerance ( float _a, float _b, float _tolerance = 0.001) {
    return std::abs(_a - _b) < _tolerance;
}

// Tests if a line segment (from point A to B) is nearly coincident with the edge of a tile
bool isOnTileEdge (const glm::vec3& _pa, const glm::vec3& _pb) {
    float tolerance = 0.0002; // tweak this adjust if catching too few/many line segments near tile edges
                                            // TODO: make tolerance configurable by source if necessary
    glm::vec2 tile_min = glm::vec2(-1.0,-1.0);
    glm::vec2 tile_max = glm::vec2(1.0,1.0);

    if (valuesWithinTolerance(_pa.x, tile_min.x, tolerance) && valuesWithinTolerance(_pb.x, tile_min.x, tolerance)) {
        return true;
    } else if (valuesWithinTolerance(_pa.x, tile_max.x, tolerance) && valuesWithinTolerance(_pb.x, tile_max.x, tolerance)) {
        return true;
    } else if (valuesWithinTolerance(_pa.y, tile_min.y, tolerance) && valuesWithinTolerance(_pb.y, tile_min.y, tolerance)) {
        return true;
    } else if (valuesWithinTolerance(_pa.y, tile_max.y, tolerance) && valuesWithinTolerance(_pb.y, tile_max.y, tolerance)) {
        return true;
    }
    return false;
}

void Builders::buildPolyLine(const Line& _line, const PolyLineOptions& _options, PolyLineOutput& _out) {
 
    int lineSize = (int)_line.size();
    
    if (lineSize < 2) {
        return;
    }
    
    // TODO: pre-allocate output vectors; try estimating worst-case space usage
    
    glm::vec3 coordPrev, coordCurr, coordNext;
    glm::vec2 normPrev, normCurr, normNext;

    int cornersOnCap = (int)_options.cap;
    int trianglesOnJoin = (int)_options.join;

    int vertexDataOffset = (int)_out.points.size();
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
        } else if (i == 0 && _options.closePolygon) {
            // If is the first point and is a close polygon
            // TODO   
            bool needToClose = true;
            if (_options.removeTileEdges) {
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
        } else if (_options.closePolygon) {
            // If is the last point a close polygon
            coordNext = _line[1];
            hasNext = true;
        }

        if (hasNext) {
            // If is not the last one get next coordinates and calculate the right normal

            normNext = glm::normalize(perp2d(coordCurr, coordNext));
            if (_options.removeTileEdges) {
                if (isOnTileEdge(coordCurr, coordNext) ) {
                    normCurr = glm::normalize(perp2d(coordPrev, coordCurr));
                    
                    if (hasPrev) {
                        addPolyLineVertex(coordCurr, normCurr, {1.0, i/(float)lineSize}, _options.halfWidth, _out);
                        addPolyLineVertex(coordCurr, normCurr, {0.0, i/(float)lineSize}, _options.halfWidth, _out);

                        // Add vertices to buffer acording their index
                        indexPairs(nPairs, vertexDataOffset, _out.indices);
                        
                        vertexDataOffset = (int)_out.points.size();
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
            if (i == 0 && !hasPrev && !_options.closePolygon) {
                // Add previus vertices to buffer and reset the index pairs counter
                // Because we are going to add more triangles.
                indexPairs(nPairs, vertexDataOffset, _out.indices);
                
                addCap( coordCurr, normCurr,
                        cornersOnCap, true, 
                        _options.halfWidth, _out);

                vertexDataOffset = (int)_out.points.size();
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
                    addPolyLineVertex(coordCurr, nA, uA, _options.halfWidth, _out);
                    addPolyLineVertex(coordCurr, nC, uC, _options.halfWidth, _out);
                } else {
                    nA = -normPrev;
                    nC = normCurr;
                    nB = -normNext;
                    uA = glm::vec2(0.0,pct);
                    uC = glm::vec2(1.0,pct);
                    uB = glm::vec2(0.0,pct);
                    addPolyLineVertex(coordCurr, nC, uC, _options.halfWidth, _out);
                    addPolyLineVertex(coordCurr, nA, uA, _options.halfWidth, _out);
                }
            
                indexPairs(nPairs, vertexDataOffset, _out.indices);

                addFan( coordCurr, nA, nC, nB, uA, uC, uB, isSigned, trianglesOnJoin, 
                       _options.halfWidth, _out);

                vertexDataOffset = (int)_out.points.size();
                nPairs = 0;

                if (isSigned) {
                    addPolyLineVertex(coordCurr, nB, uB, _options.halfWidth, _out);
                    addPolyLineVertex(coordCurr, nC, uC, _options.halfWidth, _out);
                } else {
                    addPolyLineVertex(coordCurr, nC, uC, _options.halfWidth, _out);
                    addPolyLineVertex(coordCurr, nB, uB, _options.halfWidth, _out);
                }
                
            } else {
                addPolyLineVertex( coordCurr, normCurr, {1.0, i/((float)lineSize-1.0)}, _options.halfWidth, _out);
                addPolyLineVertex( coordCurr, normCurr, {0.0, i/((float)lineSize-1.0)}, _options.halfWidth, _out);
            }
            
            if (i+1 < lineSize) {
               nPairs++;
            }

            hasPrev = true;
        }
    }
    
    // Add vertices to buffer acording their index
    indexPairs(nPairs, vertexDataOffset, _out.indices);
    

    // If is the END OF a LINE
    if(!_options.closePolygon) {
        vertexDataOffset = (int)_out.points.size();
        addCap(coordCurr, normCurr, cornersOnCap , false, _options.halfWidth, _out);
    }
}

void Builders::buildQuadAtPoint(const Point& _point, const glm::vec3& _normal, float halfWidth, float height, PolygonOutput& _out) {

}
