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

void GeometryHandler::buildPolyLine(const Line& _line, float _width, std::vector<glm::vec3>& _pointsOut, std::vector<ushort>& _indicesOut) {

    //  Normals and UV implemented but commented as:
    //
    //  _normalOut
    //  _uvOut
    
    if(_line.size() > 2){
        ushort vertexDataOffset = (ushort)_pointsOut.size();
        
        glm::vec3 upVector(0.0f, 0.0f, 1.0f);
        
        glm::vec3 normi;             // Right normal to segment between previous and current m_points
        glm::vec3 normip1;           // Right normal to segment between current and next m_points
        glm::vec3 rightNorm;         // Right "normal" at current point, scaled for miter joint
        
        glm::vec3 im1;              // Previous point coordinates
        glm::vec3 i0 = _line[0];    // Current point coordinates
        glm::vec3 ip1 = _line[1];   // Next point coordinates
        
        normip1.x = ip1.y - i0.y;
        normip1.y = i0.x - ip1.x;
        normip1.z = 0.;
        
        normip1 = glm::normalize(normip1);
        
        rightNorm = glm::vec3(normip1.x*_width,
                              normip1.y*_width,
                              normip1.z*_width);
        
        _pointsOut.push_back(i0 + rightNorm);
//        _normalOut.push_back(upVector);
//        _uvOut.push_back(glm::vec2(1.0,0.0));
        
        _pointsOut.push_back(i0 - rightNorm);
//        _normalOut.push_back(upVector);
//        _uvOut.push_back(glm::vec2(0.0,0.0));
        
        // Loop over intermediate m_points in the polyline
        //
        for (int i = 1; i < _line.size() - 1; i++) {
            im1 = i0;
            i0 = ip1;
            ip1 = _line[i+1];
            
            normi = normip1;
            normip1.x = ip1.y - i0.y;
            normip1.y = i0.x - ip1.x;
            normip1.z = 0.0f;
            normip1 = glm::normalize(normip1);
            
            rightNorm = normi + normip1;
            float scale = sqrtf(2. / (1. + glm::dot(normi,normip1) )) * _width / 2.;
            rightNorm *= scale;
            
            _pointsOut.push_back(i0+rightNorm);
//            _normalOut.push_back(upVector);
//            _uvOut.push_back(glm::vec2(1.0,(float)i/(float)_line.size()));
            
            _pointsOut.push_back(i0-rightNorm);
//            _normalOut.push_back(upVector);
//            _uvOut.push_back(glm::vec2(0.0,(float)i/(float)_line.size()));
            
        }
        
        normip1 *= _width;
        
        _pointsOut.push_back(ip1 + normip1);
//        _normalOut.push_back(upVector);
//        _uvOut.push_back(glm::vec2(1.0,1.0));
        
        _pointsOut.push_back(ip1 - normip1);
//        _normalOut.push_back(upVector);
//        _uvOut.push_back(glm::vec2(0.0,1.0));
        
        for (int i = 0; i < _line.size() - 1; i++) {
            _indicesOut.push_back(vertexDataOffset + 2*i+3);
            _indicesOut.push_back(vertexDataOffset + 2*i+2);
            _indicesOut.push_back(vertexDataOffset + 2*i);
            
            _indicesOut.push_back(vertexDataOffset + 2*i);
            _indicesOut.push_back(vertexDataOffset + 2*i+1);
            _indicesOut.push_back(vertexDataOffset + 2*i+3);
        }
    }
}

void GeometryHandler::buildQuadAtPoint(const Point& _point, const glm::vec3& _normal, float width, float height, std::vector<glm::vec3>& _pointsOut, std::vector<ushort>& _indicesOut) {

}

