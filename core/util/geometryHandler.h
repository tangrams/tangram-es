#pragma once

#include <vector>

#include "json/json.h"
#include "tesselator.h"
#include "glm/glm.hpp"

#include "util/projection.h"
#include "platform.h"


class GeometryHandler {
    TESStesselator* m_tess;
    const MapProjection* m_mapProjection;

public:
    GeometryHandler(const MapProjection& m_mapProjection);

    template<typename T>
    void polygonAddData(const Json::Value& _geomCoordinates, std::vector<T>& _vertices, std::vector<GLushort>& _indices, const glm::vec4& _rgba, const glm::dvec2& _tileOffset, double _height, double _minHeight);

    virtual ~GeometryHandler() {
        tessDeleteTess(m_tess);
    }
};

// defining template member function
template<typename T>
void GeometryHandler::polygonAddData(const Json::Value& _geomCoordinates, std::vector<T>& _vertices, std::vector<GLushort>& _indices, const glm::vec4& _rgba, const glm::dvec2& _tileOffset, double _height, double _minHeight) {
    //Get the size of the data already added
    size_t vertexDataOffset = _vertices.size();
    
    // Iterate through rings to setup the tesselator
    for(int i = 0; i < _geomCoordinates.size(); i++) {
        //extract coordinates and transform to merc (x,y) using m_mapProjection
        int ringSize = _geomCoordinates[0].size();
        std::vector<glm::dvec3> ringCoords;

        for(int j = 0; j < ringSize; j++) {
            logMsg("\n\tgeomCoord: (%f,%f)\n", _geomCoordinates[0][j][0].asFloat(), _geomCoordinates[0][j][1].asFloat());
            glm::dvec2 tmp = m_mapProjection->LonLatToMeters(glm::dvec2(_geomCoordinates[0][j][0].asFloat(), _geomCoordinates[0][j][1].asFloat()));
            logMsg("\tLonLatToMeters: (%f,%f)\n", tmp.x, tmp.y);
            glm::dvec2 meters = tmp - _tileOffset;
            logMsg("\tMeters: (%f,%f)\n", meters.x, meters.y);
            ringCoords.push_back(glm::dvec3(meters.x, meters.y, _height));
        }

        // Extrude poly
        if(_height != _minHeight) {
            glm::dvec3 upVector(0.0f,0.0f,1.0f);
            glm::dvec3 normalVector;

            for(int j = 0; j < (ringSize-1); j++) {
                normalVector = glm::cross(upVector, ringCoords[i+1] - ringCoords[i]);
                /*
                 * NOTE: Converting double to floats for the data to be sent to GPU
                 */
                // 1st vertex top
                _vertices.push_back((T){
                                        static_cast<GLfloat>(ringCoords[i].x), 
                                        static_cast<GLfloat>(ringCoords[i].y), 
                                        static_cast<GLfloat>(ringCoords[i].z), 
                                        static_cast<GLfloat>(normalVector.x), 
                                        static_cast<GLfloat>(normalVector.y), 
                                        static_cast<GLfloat>(normalVector.z), 
                                        static_cast<GLubyte>(_rgba.x),
                                        static_cast<GLubyte>(_rgba.y),
                                        static_cast<GLubyte>(_rgba.z),
                                        static_cast<GLubyte>(_rgba.w)
                                      });
                // 2nd vertex top
                _vertices.push_back((T){
                                        static_cast<GLfloat>(ringCoords[i+1].x), 
                                        static_cast<GLfloat>(ringCoords[i+1].y), 
                                        static_cast<GLfloat>(ringCoords[i+1].z), 
                                        static_cast<GLfloat>(normalVector.x), 
                                        static_cast<GLfloat>(normalVector.y), 
                                        static_cast<GLfloat>(normalVector.z), 
                                        static_cast<GLubyte>(_rgba.x),
                                        static_cast<GLubyte>(_rgba.y),
                                        static_cast<GLubyte>(_rgba.z),
                                        static_cast<GLubyte>(_rgba.w)
                                      });
                // 1st vertex bottom
                _vertices.push_back((T){
                                        static_cast<GLfloat>(ringCoords[i].x), 
                                        static_cast<GLfloat>(ringCoords[i].y), 
                                        static_cast<GLfloat>(_minHeight), 
                                        static_cast<GLfloat>(normalVector.x), 
                                        static_cast<GLfloat>(normalVector.y), 
                                        static_cast<GLfloat>(normalVector.z),
                                        static_cast<GLubyte>(_rgba.x),
                                        static_cast<GLubyte>(_rgba.y),
                                        static_cast<GLubyte>(_rgba.z),
                                        static_cast<GLubyte>(_rgba.w)
                                      });
                // 2nd vertex bottom
                _vertices.push_back((T){
                                        static_cast<GLfloat>(ringCoords[i+1].x), 
                                        static_cast<GLfloat>(ringCoords[i+1].y), 
                                        static_cast<GLfloat>(_minHeight), 
                                        static_cast<GLfloat>(normalVector.x), 
                                        static_cast<GLfloat>(normalVector.y), 
                                        static_cast<GLfloat>(normalVector.z), 
                                        static_cast<GLubyte>(_rgba.x),
                                        static_cast<GLubyte>(_rgba.y),
                                        static_cast<GLubyte>(_rgba.z),
                                        static_cast<GLubyte>(_rgba.w)
                                      });
                
                //Start the index from the previous state of the vertex Data
                _indices.push_back(vertexDataOffset);
                _indices.push_back(vertexDataOffset + 2);
                _indices.push_back(vertexDataOffset + 1);

                _indices.push_back(vertexDataOffset + 1);
                _indices.push_back(vertexDataOffset + 2);
                _indices.push_back(vertexDataOffset + 3);

                vertexDataOffset += 4;
            }
        }
        //Setup the tesselator
        tessAddContour(m_tess, 3, &ringCoords[0].x, sizeof(glm::dvec3), ringSize);
    }

    //Call the tesselator to tesselate polygon into triangles
    if( tessTesselate(m_tess, TessWindingRule::TESS_WINDING_NONZERO, TessElementType::TESS_POLYGONS, 3, 3, nullptr)) {
        const int numIndices = tessGetElementCount(m_tess);
        const TESSindex* tessIndices = tessGetElements(m_tess);

        for(int i = 0; i < numIndices; i++) {
            const TESSindex* tessIndex = &tessIndices[i * 3];
            for(int j = 0; j < 3; j++) {
                _indices.push_back(GLubyte(tessIndex[j]) + vertexDataOffset);
            }
        }

        const int numVertices = tessGetVertexCount(m_tess);
        const float* tessVertices = tessGetVertices(m_tess);

        for(int i = 0; i < numVertices; i++) {
            _vertices.push_back((T){
                                    tessVertices[3*i],
                                    tessVertices[3*i+1],
                                    tessVertices[3*i+2],
                                    0.0f, 0.0f, 1.0f,
                                    static_cast<GLubyte>(_rgba.x),
                                    static_cast<GLubyte>(_rgba.y),
                                    static_cast<GLubyte>(_rgba.z),
                                    static_cast<GLubyte>(_rgba.w)
                                   });
        }
    }
    else {
        logMsg("\t\t****tessTessalate returns false. Can not tesselate.****\n");
    }
}

