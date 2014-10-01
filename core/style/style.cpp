#include "style.h"
#include "platform.h"

/*
 * Style Class Methods
 */
Style::Style(std::string _geomType, GLenum _drawMode) : m_geomType(_geomType), m_styleName(_geomType), m_drawMode(_drawMode) {
    m_tess = tessNewTess(nullptr);
}

Style::Style(std::string _geomType, std::string _styleName, GLenum _drawMode) : m_geomType(_geomType), m_drawMode(_drawMode) {
}

void Style::setFragShaderSrc(std::string _fragShaderSrcStr) {
    m_fragShaderSrcStr = std::move(_fragShaderSrcStr);
}

void Style::setVertShaderSrc(std::string _vertShaderSrcStr) {
    m_vertShaderSrcStr = std::move(_vertShaderSrcStr);
}

std::string Style::getStyleName() {
    return m_styleName;
}

std::string Style::getGeometryType() {
    return m_geomType;
}

std::shared_ptr<ShaderProgram> Style::getShaderProgram() {
    return m_shaderProgram;
}

/*
 * Polygon Style Class Methods
 */
PolygonStyle::PolygonStyle(std::string _geomType, GLenum _drawMode) : Style(_geomType, _drawMode) {
    
    // Pass in a fileName or a url to construct shader strings from
    m_vertShaderSrcStr =
        "#ifdef GL_ES\n"
        "precision mediump float;\n"
        "#endif\n"
        "attribute vec4 a_position;\n"
        "attribute vec4 a_color;\n"
        "varying vec4 v_color;\n"
        "void main() {\n"
        "  v_color = a_color;\n"
        "  gl_Position = a_position;\n"
        "}\n";

    m_fragShaderSrcStr =
        "#ifdef GL_ES\n"
        "precision mediump float;\n"
        "#endif\n"
        "varying vec4 v_color;\n"
        "void main(void) {\n"
        "  gl_FragColor = v_color;\n"
        "}\n";
    constructVertexLayout();
    constructShaderProgram();
}

PolygonStyle::PolygonStyle(std::string _geomType, std::string _styleName, GLenum _drawMode) : Style(_geomType, _styleName, _drawMode) {
    
    // Pass in a fileName or a url to construct shader strings from
    m_vertShaderSrcStr =
        "#ifdef GL_ES\n"
        "precision mediump float;\n"
        "#endif\n"
        "attribute vec4 a_position;\n"
        "attribute vec4 a_color;\n"
        "varying vec4 v_color;\n"
        "void main() {\n"
        "  v_color = a_color;\n"
        "  gl_Position = a_position;\n"
        "}\n";

    m_fragShaderSrcStr =
        "#ifdef GL_ES\n"
        "precision mediump float;\n"
        "#endif\n"
        "varying vec4 v_color;\n"
        "void main(void) {\n"
        "  gl_FragColor = v_color;\n"
        "}\n";
    constructVertexLayout();
    constructShaderProgram();
}

void PolygonStyle::constructVertexLayout() {
    // fixed per style... this is basic position, normal color
    m_vertexLayout = std::shared_ptr<VertexLayout>(new VertexLayout({
        {"a_position", 3, GL_FLOAT, false, 0},
        {"a_normal", 3, GL_FLOAT, false, 0},
        {"a_color", 4, GL_UNSIGNED_BYTE, true, 0}
    }));
}

void PolygonStyle::constructShaderProgram() {
    m_shaderProgram = std::shared_ptr<ShaderProgram>(new ShaderProgram());
    m_shaderProgram->buildFromSourceStrings(m_fragShaderSrcStr, m_vertShaderSrcStr);
}

void PolygonStyle::addDataToVBO(Json::Value _geomCoordinates, Json::Value _property, glm::vec4& _color, std::shared_ptr<VboMesh> _vboMesh, glm::vec2 _tileOffset, MapProjection& _mapProjection) {
    
    //Set Layout and drawmode for the _vboMesh
    _vboMesh->setVertexLayout(m_vertexLayout);
    _vboMesh->setDrawMode(m_drawMode);
    //float color to usigned bytes
    GLubyte colorR = static_cast<GLubyte>(_color.x * 255.0f);
    GLubyte colorG = static_cast<GLubyte>(_color.y * 255.0f);
    GLubyte colorB = static_cast<GLubyte>(_color.z * 255.0f);
    GLubyte colorA = static_cast<GLubyte>(_color.w * 255.0f);

    //Extract Feature Properties
    float featureHeight  = 0.0f;
    float minFeatureHeight = 0.0f;
    if (_property.isMember("height")) {
        featureHeight = _property["height"].asFloat();
    }
    if (_property.isMember("min_height")) {
        minFeatureHeight = _property["min_height"].asFloat();
    }
    
    //Setup for tesselator
    size_t vertexDataOffset = m_vertices.size();
    //Loop all poly rings
    for(int i = 0; i < _geomCoordinates.size(); i++) {
        //extract coordinates and transform to merc (x,y) using _mapProjection
        int ringSize = _geomCoordinates[0].size();
        std::vector<glm::vec3> ringCoords;

        for(int j = 0; j < ringSize; j++) {
            glm::vec2 meters = _mapProjection.LatLonToMeters(glm::vec2(_geomCoordinates[0][i][0].asFloat(), _geomCoordinates[0][i][1].asFloat())) - _tileOffset;
            ringCoords.push_back(glm::vec3(meters.x, meters.y, featureHeight));
        }

        // Extrude poly
        if(featureHeight != minFeatureHeight) {
            glm::vec3 upVector(0.0f,0.0f,1.0f);
            glm::vec3 normalVector;

            for(int j = 0; j < (ringSize-1); j++) {
                normalVector = glm::cross(upVector, ringCoords[i+1] - ringCoords[i]);
                // 1st vertex top
                m_vertices.push_back((vboDataUnit){ringCoords[i].x, ringCoords[i].y, ringCoords[i].z, normalVector.x, normalVector.y, normalVector.z, colorR, colorG, colorB, colorA});
                // 2nd vertex top
                m_vertices.push_back((vboDataUnit){ringCoords[i+1].x, ringCoords[i+1].y, ringCoords[i+1].z, normalVector.x, normalVector.y, normalVector.z, colorR, colorG, colorB, colorA});
                // 1st vertex bottom
                m_vertices.push_back((vboDataUnit){ringCoords[i].x, ringCoords[i].y, minFeatureHeight, normalVector.x, normalVector.y, normalVector.z, colorR, colorG, colorB, colorA});
                // 2nd vertex bottom
                m_vertices.push_back((vboDataUnit){ringCoords[i+1].x, ringCoords[i+1].y, minFeatureHeight, normalVector.x, normalVector.y, normalVector.z, colorR, colorG, colorB, colorA});

                m_indices.push_back(vertexDataOffset);
                m_indices.push_back(vertexDataOffset + 2);
                m_indices.push_back(vertexDataOffset + 1);

                m_indices.push_back(vertexDataOffset + 1);
                m_indices.push_back(vertexDataOffset + 2);
                m_indices.push_back(vertexDataOffset + 3);

                vertexDataOffset += 4;

            }
        }
        
        tessAddContour(m_tess, 3, &ringCoords[0].x, sizeof(glm::vec3), ringSize);
    }

    //Call the tesselator to tesselate polygon into triangles
    tessTesselate(m_tess, TESS_WINDING_NONZERO, TessElementType::TESS_POLYGONS, 3, 3, nullptr);

    const int numIndices = tessGetElementCount(m_tess);
    const TESSindex* tessIndices = tessGetElements(m_tess);

    for(int i = 0; i < numIndices; i++) {
        const TESSindex* tessIndex = &tessIndices[i * 3];
        for(int j = 0; j < 3; j++) {
            m_indices.push_back(GLubyte(tessIndex[j]) + vertexDataOffset);
        }
    }

    const int numVertices = tessGetVertexCount(m_tess);
    const float* tessVertices = tessGetVertices(m_tess);

    for(int i = 0; i < numVertices; i++) {
        m_vertices.push_back((vboDataUnit){tessVertices[3*i], tessVertices[3*i+1], tessVertices[3*i+2], 0.0f, 0.0f, 1.0f, colorR, colorG, colorB, colorA});
    }
}


