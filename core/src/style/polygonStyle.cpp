#include "polygonStyle.h"
#include "util/builders.h"

PolygonStyle::PolygonStyle(std::string _name, GLenum _drawMode) : Style(_name, _drawMode) {
    
    constructVertexLayout();
    constructShaderProgram();
}

void PolygonStyle::constructVertexLayout() {
    
    // TODO: Ideally this would be in the same location as the struct that it basically describes
    m_vertexLayout = std::shared_ptr<VertexLayout>(new VertexLayout({
        {"a_position", 3, GL_FLOAT, false, 0},
        {"a_normal", 3, GL_FLOAT, false, 0},
        {"a_texcoord", 2, GL_FLOAT, false, 0},
        {"a_color", 4, GL_UNSIGNED_BYTE, true, 0}
    }));
    
}

void PolygonStyle::constructShaderProgram() {
    
    std::string vertShaderSrcStr = stringFromResource("polygon.vs");
    std::string fragShaderSrcStr = stringFromResource("polygon.fs");
    
    m_shaderProgram = std::make_shared<ShaderProgram>();
    m_shaderProgram->loadSourceStrings(fragShaderSrcStr, vertShaderSrcStr);

    m_material.enableSpecular();
    m_shaderProgram->addBlock("material",m_material.getBlock());   // This is a must for lighting !!
}

void PolygonStyle::setup() {
    m_material.setupProgram(m_shaderProgram.get());
}

void PolygonStyle::buildPoint(Point& _point, std::string& _layer, Properties& _props, VboMesh& _mesh) {
    // No-op
}

void PolygonStyle::buildLine(Line& _line, std::string& _layer, Properties& _props, VboMesh& _mesh) {
    std::vector<PosNormColVertex> vertices;
    std::vector<GLushort> indices;
    std::vector<glm::vec3> points;
    std::vector<glm::vec2> texcoords;
    
    GLuint abgr = 0xff969696; // Default road color
    float halfWidth = 0.02;
    
    Builders::buildPolyLine(_line, halfWidth, points, indices, texcoords);
    
    for (size_t i = 0; i < points.size(); i++) {
        glm::vec3 p = points[i];
        glm::vec3 n = glm::vec3(0.0f, 0.0f, 1.0f);
        glm::vec2 u = texcoords[i];
        vertices.push_back({ p.x, p.y, p.z, n.x, n.y, n.z, u.x, u.y, abgr });
    }
    
    // Make sure indices get correctly offset
    int vertOffset = _mesh.numVertices();
    for (auto& ind : indices) {
        ind += vertOffset;
    }
    
    _mesh.addVertices((GLbyte*)vertices.data(), vertices.size());
    _mesh.addIndices(indices.data(), indices.size());
}

void PolygonStyle::buildPolygon(Polygon& _polygon, std::string& _layer, Properties& _props, VboMesh& _mesh) {
    
    std::vector<PosNormColVertex> vertices;
    std::vector<GLushort> indices;
    std::vector<glm::vec3> points;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texcoords;
    
    GLuint abgr = 0xffaaaaaa; // Default color
    
    if (_layer.compare("buildings") == 0) {
        abgr = 0xffcedcde;
    } else if (_layer.compare("water") == 0) {
        abgr = 0xff917d1a;
    } else if (_layer.compare("roads") == 0) {
        abgr = 0xff969696;
    } else if (_layer.compare("earth") == 0) {
        abgr = 0xff669171;
    } else if (_layer.compare("landuse") == 0) {
        abgr = 0xff507480;
    }
    
    float height = _props.numericProps["height"]; // Inits to zero if not present in data
    float minHeight = _props.numericProps["min_height"]; // Inits to zero if not present in data
    
    if (minHeight != height) {
        for (auto& line : _polygon) {
            for (auto& point : line) {
                point.z = height;
            }
        }
        Builders::buildPolygonExtrusion(_polygon, minHeight, points, normals, indices, texcoords);
    }
    
    Builders::buildPolygon(_polygon, points, normals, indices, texcoords);
    
    for (size_t i = 0; i < points.size(); i++) {
        glm::vec3 p = points[i];
        glm::vec3 n = normals[i];
        glm::vec2 u = texcoords[i];
        vertices.push_back({ p.x, p.y, p.z, n.x, n.y, n.z, u.x, u.y, abgr });
    }
    
    // Make sure indices get correctly offset
    int vertOffset = _mesh.numVertices();
    for (auto& ind : indices) {
        ind += vertOffset;
    }
    
    _mesh.addVertices((GLbyte*)vertices.data(), vertices.size());
    _mesh.addIndices(indices.data(), indices.size());
}
