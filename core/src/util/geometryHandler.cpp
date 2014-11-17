#include "geometryHandler.h"

#include "tesselator.h"

void GeometryHandler::buildPolygon(const Polygon& _polygon, std::vector<glm::vec3>& _pointsOut, std::vector<glm::vec3>& _normalOut, std::vector<ushort>& _indicesOut) {
    
    TESStesselator* tesselator = tessNewTess(nullptr);

    // get the number of vertices already added
    ushort vertexDataOffset = (ushort)_pointsOut.size();
    
    // add polygon contour for every ring
    for (auto& line : _polygon) {
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
            _pointsOut.push_back(glm::vec3(tessVertices[3*i], tessVertices[3*i+1], tessVertices[3*i+2]));
            _normalOut.push_back(normal);
        }
    }
    else {
        logMsg("Tesselator cannot tesselate!!\n");
    }
    
    tessDeleteTess(tesselator);
}

void GeometryHandler::buildPolygonExtrusion(const Polygon& _polygon, const float& _minHeight, std::vector<glm::vec3>& _pointsOut, std::vector<glm::vec3>& _normalOut, std::vector<ushort>& _indicesOut) {
    
    ushort vertexDataOffset = (ushort)_pointsOut.size();
    
    glm::vec3 upVector(0.0f, 0.0f, 1.0f);
    glm::vec3 normalVector;

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

            vertexDataOffset += 4;

        }
    }
}

void GeometryHandler::buildPolyLine(const Line& _line, float _halfWidth, std::vector<glm::vec3>& _pointsOut, std::vector<ushort>& _indicesOut) {

    //  UV implemented but commented as: _uvOut
    //
    
    ushort vertexDataOffset = (ushort)_pointsOut.size();
    
    if(_line.size() >= 2){
        
        glm::vec3 normPrevCurr;             // Right normal to segment between previous and current m_points
        glm::vec3 normCurrNext;           // Right normal to segment between current and next m_points
        glm::vec3 rightNorm;         // Right "normal" at current point, scaled for miter joint
        
        glm::vec3 prevCoord;              // Previous point coordinates
        glm::vec3 currCoord = _line[0];    // Current point coordinates
        glm::vec3 nextCoord = _line[1];   // Next point coordinates
    
        normCurrNext.x = nextCoord.y - currCoord.y;
        normCurrNext.y = currCoord.x - nextCoord.x;
        normCurrNext.z = 0.;
        normCurrNext = glm::normalize(normCurrNext);
        
        
        rightNorm = glm::vec3(normCurrNext.x*_halfWidth,
                              normCurrNext.y*_halfWidth,
                              normCurrNext.z*_halfWidth);
        
        _pointsOut.push_back(currCoord + rightNorm);
//        _uvOut.push_back(glm::vec2(1.0,0.0));
        
        _pointsOut.push_back(currCoord - rightNorm);
//        _uvOut.push_back(glm::vec2(0.0,0.0));
        
        // Loop over intermediate m_points in the polyline
        //
        for (int i = 1; i < _line.size() - 1; i++) {
            prevCoord = currCoord;
            currCoord = nextCoord;
            nextCoord = _line[i+1];
            
            normPrevCurr = normCurrNext;
            
            normCurrNext.x = nextCoord.y - currCoord.y;
            normCurrNext.y = currCoord.x - nextCoord.x;
            normCurrNext.z = 0.0f;
            
            rightNorm = normPrevCurr + normCurrNext;
            rightNorm = glm::normalize(rightNorm);
            float scale = sqrtf(2. / (1. + glm::dot(normPrevCurr,normCurrNext) )) * _halfWidth / 2.;
            rightNorm *= scale;
            
            _pointsOut.push_back(currCoord+rightNorm);
//            _uvOut.push_back(glm::vec2(1.0,(float)i/(float)_line.size()));
            
            _pointsOut.push_back(currCoord-rightNorm);
//            _uvOut.push_back(glm::vec2(0.0,(float)i/(float)_line.size()));
            
        }
        
        normCurrNext = glm::normalize(normCurrNext);
        normCurrNext *= _halfWidth;
        
        _pointsOut.push_back(nextCoord + normCurrNext);
//        _uvOut.push_back(glm::vec2(1.0,1.0));
        
        _pointsOut.push_back(nextCoord - normCurrNext);
//        _uvOut.push_back(glm::vec2(0.0,1.0));
        
        for (int i = 0; i < _line.size() - 1; i++) {
            _indicesOut.push_back(vertexDataOffset + 2*i+2);
            _indicesOut.push_back(vertexDataOffset + 2*i+1);
            _indicesOut.push_back(vertexDataOffset + 2*i);
            
            _indicesOut.push_back(vertexDataOffset + 2*i+2);
            _indicesOut.push_back(vertexDataOffset + 2*i+3);
            _indicesOut.push_back(vertexDataOffset + 2*i+1);
        }
    }
}

void GeometryHandler::buildQuadAtPoint(const Point& _point, const glm::vec3& _normal, float halfWidth, float height, std::vector<glm::vec3>& _pointsOut, std::vector<ushort>& _indicesOut) {

}

