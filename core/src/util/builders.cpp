#include "builders.h"

#include "tesselator.h"
#include "rectangle.h"
#include "geom.h"

static auto& NO_TEXCOORDS = *(new std::vector<glm::vec2>); // denotes that texture coordinates should not be used
static auto& NO_SCALING_VECS = *(new std::vector<glm::vec2>); // denotes that scaling vectors should not be used

void Builders::buildPolygon(const Polygon& _polygon, std::vector<glm::vec3>& _pointsOut, std::vector<glm::vec3>& _normalOut, std::vector<ushort>& _indicesOut) {
    
    buildPolygon(_polygon, _pointsOut, _normalOut, _indicesOut, NO_TEXCOORDS);
    
}

void Builders::buildPolygon(const Polygon& _polygon, std::vector<glm::vec3>& _pointsOut, std::vector<glm::vec3>& _normalOut, std::vector<ushort>& _indicesOut, std::vector<glm::vec2>& _texcoordOut) {
    
    TESStesselator* tesselator = tessNewTess(nullptr);
    
    bool useTexCoords = &_texcoordOut != &NO_TEXCOORDS;
    
    // get the number of vertices already added
    ushort vertexDataOffset = (ushort)_pointsOut.size();
    
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
        tessAddContour(tesselator, 3, line.data(), sizeof(Point), line.size());
    }
    
    // call the tesselator
    glm::vec3 normal(0.0, 0.0, 1.0);
    
    if( tessTesselate(tesselator, TessWindingRule::TESS_WINDING_NONZERO, TessElementType::TESS_POLYGONS, 3, 3, &normal[0]) ) {
        
        const int numElements = tessGetElementCount(tesselator);
        const TESSindex* tessElements = tessGetElements(tesselator);
        for(int i = 0; i < numElements; i++) {
            const TESSindex* tessElement = &tessElements[i * 3];
            for(int j = 0; j < 3; j++) {
                _indicesOut.push_back((ushort)tessElement[j] + vertexDataOffset);
            }
        }
        
        const int numVertices = tessGetVertexCount(tesselator);
        const float* tessVertices = tessGetVertices(tesselator);
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

void Builders::buildPolygonExtrusion(const Polygon& _polygon, const float& _minHeight, std::vector<glm::vec3>& _pointsOut, std::vector<glm::vec3>& _normalOut, std::vector<ushort>& _indicesOut) {
    
    buildPolygonExtrusion(_polygon, _minHeight, _pointsOut, _normalOut, _indicesOut, NO_TEXCOORDS);
    
}

void Builders::buildPolygonExtrusion(const Polygon& _polygon, const float& _minHeight, std::vector<glm::vec3>& _pointsOut, std::vector<glm::vec3>& _normalOut, std::vector<ushort>& _indicesOut, std::vector<glm::vec2>& _texcoordOut) {
    
    ushort vertexDataOffset = (ushort)_pointsOut.size();
    
    glm::vec3 upVector(0.0f, 0.0f, 1.0f);
    glm::vec3 normalVector;
    
    bool useTexCoords = &_texcoordOut != &NO_TEXCOORDS;
    
    for(auto& line : _polygon) {
        
        for(int i = 0; i < line.size() - 1; i++) {
            
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

void buildGeneralPolyLine(const Line& _line, float _halfWidth, std::vector<glm::vec3>& _pointsOut, std::vector<glm::vec2>& _scalingVecsOut, std::vector<Builders::ushort>& _indicesOut, std::vector<glm::vec2>& _texCoordOut) {

    using Builders::ushort;
    
    if (_line.size() < 2) {
        return;
    }
    
    ushort vertexDataOffset = (ushort)_pointsOut.size();
    
    bool useTexCoords = &_texCoordOut != &NO_TEXCOORDS;
    bool useScalingVecs = &_scalingVecsOut != &NO_SCALING_VECS;
    
    glm::vec2 normPrevCurr; // Right normal to segment between previous and current m_points
    glm::vec2 normCurrNext; // Right normal to segment between current and next m_points
    glm::vec2 rightNorm; // Right "normal" at current point, scaled for miter joint
    
    glm::vec3 prevCoord; // Previous point coordinates
    glm::vec3 currCoord = _line[0]; // Current point coordinates
    glm::vec3 nextCoord = _line[1]; // Next point coordinates
    
    normCurrNext.x = nextCoord.y - currCoord.y;
    normCurrNext.y = currCoord.x - nextCoord.x;
    normCurrNext = glm::normalize(normCurrNext);
    
    rightNorm = normCurrNext;
    
    if (useScalingVecs) {
        _pointsOut.push_back(currCoord);
        _pointsOut.push_back(currCoord);
        _scalingVecsOut.push_back(rightNorm);
        _scalingVecsOut.push_back(-rightNorm);
    } else {
        _pointsOut.push_back(glm::vec3(currCoord.x + rightNorm.x * _halfWidth, currCoord.y + rightNorm.y * _halfWidth, currCoord.z));
        _pointsOut.push_back(glm::vec3(currCoord.x - rightNorm.x * _halfWidth, currCoord.y - rightNorm.y * _halfWidth, currCoord.z));
    }
    
    if (useTexCoords) {
        _texCoordOut.push_back(glm::vec2(1.0,0.0));
        _texCoordOut.push_back(glm::vec2(0.0,0.0));
    }
    
    // Loop over intermediate points in the polyline
    for (int i = 1; i < _line.size() - 1; i++) {
        prevCoord = currCoord;
        currCoord = nextCoord;
        nextCoord = _line[i+1];
        
        normPrevCurr = normCurrNext;
        
        normCurrNext.x = nextCoord.y - currCoord.y;
        normCurrNext.y = currCoord.x - nextCoord.x;
        
        rightNorm = normPrevCurr + normCurrNext;
        rightNorm = glm::normalize(rightNorm);
        float scale = sqrtf(2. / (1. + glm::dot(normPrevCurr,normCurrNext) ));
        rightNorm *= scale;
        
        if (useScalingVecs) {
            _pointsOut.push_back(currCoord);
            _pointsOut.push_back(currCoord);
            _scalingVecsOut.push_back(rightNorm);
            _scalingVecsOut.push_back(-rightNorm);
        } else {
            _pointsOut.push_back(glm::vec3(currCoord.x + rightNorm.x * _halfWidth, currCoord.y + rightNorm.y * _halfWidth, currCoord.z));
            _pointsOut.push_back(glm::vec3(currCoord.x - rightNorm.x * _halfWidth, currCoord.y - rightNorm.y * _halfWidth, currCoord.z));
        }
        
        if (useTexCoords) {
            _texCoordOut.push_back(glm::vec2(1.0, i/(float)_line.size()));
            _texCoordOut.push_back(glm::vec2(0.0, i/(float)_line.size()));
        }
        
    }
    
    normCurrNext = glm::normalize(normCurrNext);
    
    _pointsOut.push_back(nextCoord);
    _pointsOut.push_back(nextCoord);
    
    if (useScalingVecs) {
        _scalingVecsOut.push_back(rightNorm);
        _scalingVecsOut.push_back(-rightNorm);
    } else {
        _pointsOut.push_back(glm::vec3(currCoord.x + rightNorm.x * _halfWidth, currCoord.y + rightNorm.y * _halfWidth, currCoord.z));
        _pointsOut.push_back(glm::vec3(currCoord.x - rightNorm.x * _halfWidth, currCoord.y - rightNorm.y * _halfWidth, currCoord.z));
    }
    
    if (useTexCoords) {
        _texCoordOut.push_back(glm::vec2(1.0,1.0));
        _texCoordOut.push_back(glm::vec2(0.0,1.0));
    }
    
    for (int i = 0; i < _line.size() - 1; i++) {
        _indicesOut.push_back(vertexDataOffset + 2*i+2);
        _indicesOut.push_back(vertexDataOffset + 2*i+1);
        _indicesOut.push_back(vertexDataOffset + 2*i);
        
        _indicesOut.push_back(vertexDataOffset + 2*i+2);
        _indicesOut.push_back(vertexDataOffset + 2*i+3);
        _indicesOut.push_back(vertexDataOffset + 2*i+1);
    }
    
}

void Builders::buildPolyLine(const Line& _line, float _halfWidth, std::vector<glm::vec3>& _pointsOut, std::vector<ushort>& _indicesOut) {

    buildGeneralPolyLine(_line, _halfWidth, _pointsOut, NO_SCALING_VECS, _indicesOut, NO_TEXCOORDS);
    
}

void Builders::buildPolyLine(const Line& _line, float _halfWidth, std::vector<glm::vec3>& _pointsOut, std::vector<ushort>& _indicesOut, std::vector<glm::vec2>& _texcoordOut) {
    
    buildGeneralPolyLine(_line, _halfWidth, _pointsOut, NO_SCALING_VECS, _indicesOut, _texcoordOut);
    
}

void Builders::buildScalablePolyLine(const Line& _line, std::vector<glm::vec3>& _pointsOut, std::vector<glm::vec2>& _scalingVecsOut, std::vector<ushort>& _indicesOut) {
    
    buildGeneralPolyLine(_line, 0, _pointsOut, _scalingVecsOut, _indicesOut, NO_TEXCOORDS);
    
}

void Builders::buildScalablePolyLine(const Line& _line, std::vector<glm::vec3>& _pointsOut, std::vector<glm::vec2>& _scalingVecsOut, std::vector<ushort>& _indicesOut, std::vector<glm::vec2>& _texcoordOut) {
    
    buildGeneralPolyLine(_line, 0, _pointsOut, _scalingVecsOut, _indicesOut, _texcoordOut);
}

void Builders::buildQuadAtPoint(const Point& _point, const glm::vec3& _normal, float halfWidth, float height, std::vector<glm::vec3>& _pointsOut, std::vector<ushort>& _indicesOut) {

}
