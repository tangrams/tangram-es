#include "geometryHandler.h"

//static TESStesselator* GeometryHandler::tesselator = nullptr;

void GeometryHandler::init() {
    // might not need this, can just check if tesselator is valid in each context that uses it
    if (tesselator == nullptr) {
        //create a tesselator with null allocator (default implementation as per libtess2)
        tesselator = tessNewTess(nullptr);
    }
}

void GeometryHandler::cleanup() {
    if (tesselator != nullptr) {
        tessDeleteTess(tesselator);
    }
}

void GeometryHandler::buildPolygon(const std::vector<glm::vec3>& _pointsIn, const std::vector<int>& _ringSizes, std::vector<glm::vec3>& _pointsOut, std::vector<glm::vec3>& _normalOut, std::vector<GLushort>& _indicesOut) {
    
    //Get the number of vertices already added
    GLushort vertexDataOffset = (GLushort)_pointsOut.size();
    
    // add polygon contour for every ring
    int ringIndex = 0;
    for(int i = 0; i < _ringSizes.size(); i++) {
        tessAddContour(tesselator, 3, &_pointsIn[ringIndex].x, sizeof(glm::vec3), _ringSizes[i]);
        ringIndex = _ringSizes[i];
    }

    //call the tesselator
    if( tessTesselate(tesselator, TessWindingRule::TESS_WINDING_NONZERO, TessElementType::TESS_POLYGONS, 3, 3, nullptr) ) {

        const int numElements = tessGetElementCount(tesselator);
        const TESSindex* tessElements = tessGetElements(tesselator);
        for(int i = 0; i < numElements; i++) {
            const TESSindex* tessElement = &tessElements[i * 3];
            for(int j = 0; j < 3; j++) {
                _indicesOut.push_back((GLushort)tessElement[j] + vertexDataOffset);
            }
        }

        const int numVertices = tessGetVertexCount(tesselator);
        const float* tessVertices = tessGetVertices(tesselator);
        for(int i = 0; i < numVertices; i++) {
            _pointsOut.push_back(glm::vec3(tessVertices[3*i], tessVertices[3*i+1], tessVertices[3*i+2]));
            _normalOut.push_back(glm::vec3(0.0, 0.0, 1.0));
        }
    }
    else {
        logMsg("\t\t***tessTesselate returns false. Can not tesselate.****\n");
    }
}

void GeometryHandler::buildPolygonExtrusion(const std::vector<glm::vec3>& _pointsIn, const std::vector<int>& _ringSizes, const float& _minFeatureHeight, std::vector<glm::vec3>& _pointsOut, std::vector<glm::vec3>& _normalOut, std::vector<GLushort>& _indicesOut) {
    GLushort vertexDataOffset = (GLushort)_pointsOut.size();

    glm::vec3 upVector(0.0f, 0.0f, 1.0f);
    glm::vec3 normalVector;

    int ringIndex = 0;

    for(int i = 0; i < _ringSizes.size(); i++) {
        for(int j = ringIndex; j < (_ringSizes[i]-1); j++) {
            normalVector = glm::cross(upVector, (_pointsIn[j+1] - _pointsIn[j]));
            normalVector = glm::normalize(normalVector);
            // 1st vertex top
            _pointsOut.push_back(glm::vec3(_pointsIn[j].x, _pointsIn[j].y, _pointsIn[j].z));
            _normalOut.push_back(glm::vec3(normalVector.x, normalVector.y, normalVector.z));
            // 2nd vertex top
            _pointsOut.push_back(glm::vec3(_pointsIn[j+1].x, _pointsIn[j+1].y, _pointsIn[j].z));
            _normalOut.push_back(glm::vec3(normalVector.x, normalVector.y, normalVector.z));
            // 1st vertex bottom
            _pointsOut.push_back(glm::vec3(_pointsIn[j].x, _pointsIn[j].y, _minFeatureHeight));
            _normalOut.push_back(glm::vec3(normalVector.x, normalVector.y, normalVector.z));
            // 2nd vertex bottom
            _pointsOut.push_back(glm::vec3(_pointsIn[j+1].x, _pointsIn[j+1].y, _minFeatureHeight));
            _normalOut.push_back(glm::vec3(normalVector.x, normalVector.y, normalVector.z));
            
            //Start the index from the previous state of the vertex Data
            _indicesOut.push_back(vertexDataOffset);
            _indicesOut.push_back(vertexDataOffset + 2);
            _indicesOut.push_back(vertexDataOffset + 1);

            _indicesOut.push_back(vertexDataOffset + 1);
            _indicesOut.push_back(vertexDataOffset + 2);
            _indicesOut.push_back(vertexDataOffset + 3);

            vertexDataOffset += 4;

        }
    }
}

void GeometryHandler::buildPolyLine(const std::vector<glm::vec3>& _pointsIn, float width, std::vector<glm::vec3>& _pointsOut, std::vector<GLushort>& _indicesOut) {

}

void GeometryHandler::buildQuadAtPoint(const glm::vec3& _pointIn, const glm::vec3& _normal, float width, float height, std::vector<glm::vec3>& _pointsOut, std::vector<GLushort>& _indicesOut) {

}
