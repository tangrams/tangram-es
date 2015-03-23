#include "polylineStyle.h"
#include "util/builders.h"
#include "roadLayers.h"
#include <ctime>

PolylineStyle::PolylineStyle(std::string _name, GLenum _drawMode) : Style(_name, _drawMode) {
    constructVertexLayout();
    constructShaderProgram();
}

void PolylineStyle::constructVertexLayout() {
    
    // TODO: Ideally this would be in the same location as the struct that it basically describes
    m_vertexLayout = std::shared_ptr<VertexLayout>(new VertexLayout({
        {"a_position", 3, GL_FLOAT, false, 0},
        {"a_texcoord", 2, GL_FLOAT, false, 0},
        {"a_extrudeNormal", 2, GL_FLOAT, false, 0},
        {"a_extrudeWidth", 1, GL_FLOAT, false, 0},
        {"a_color", 4, GL_UNSIGNED_BYTE, true, 0},
        {"a_layer", 1, GL_FLOAT, false, 0}
    }));
    
}

void PolylineStyle::constructShaderProgram() {
    
    std::string vertShaderSrcStr = stringFromResource("polyline.vs");
    std::string fragShaderSrcStr = stringFromResource("polyline.fs");
    
    m_shaderProgram = std::make_shared<ShaderProgram>();
    m_shaderProgram->setSourceStrings(fragShaderSrcStr, vertShaderSrcStr);
}

void PolylineStyle::buildPoint(Point& _point, std::string& _layer, Properties& _props, VboMesh& _mesh) const {
    // No-op
}

void PolylineStyle::buildLine(Line& _line, std::string& _layer, Properties& _props, VboMesh& _mesh) const {
    std::vector<PosNormEnormColVertex> vertices;
    std::vector<int> indices;
    std::vector<glm::vec3> points;
    std::vector<glm::vec2> texcoords;
    std::vector<glm::vec2> scalingVecs;
    
    GLuint abgr = 0xff767676; // Default road color
    GLfloat layer = sortKeyToLayer(_props.numericProps["sort_key"]) + 3;
    
    float halfWidth = 0.02;
    
    const std::string& kind = _props.stringProps["kind"];
    
    if (kind == "highway") {
        halfWidth = 0.02;
    } else if (kind == "major_road") {
        halfWidth = 0.015;
    } else if (kind == "minor_road") {
        halfWidth = 0.01;
    } else if (kind == "rail") {
        halfWidth = 0.002;
    } else if (kind == "path") {
        halfWidth = 0.005;
    }
    
    PolyLineOutput lineOutput = { points, indices, scalingVecs, texcoords };
    PolyLineOptions lineOptions = { CapTypes::ROUND, JoinTypes::ROUND, halfWidth };
    Builders::buildPolyLine(_line, lineOptions, lineOutput);
    
    for (size_t i = 0; i < points.size(); i++) {
        const glm::vec3& p = points[i];
        const glm::vec2& uv = texcoords[i];
        const glm::vec2& en = scalingVecs[i];
        vertices.push_back({ p.x, p.y, p.z, uv.x, uv.y, en.x, en.y, halfWidth, abgr, layer });
    }
    
    // Make sure indices get correctly offset
    int vertOffset = _mesh.numVertices();
    for (auto& ind : indices) {
        ind += vertOffset;
    }
    
    _mesh.addVertices((GLbyte*)vertices.data(), (int)vertices.size());
    _mesh.addIndices(indices.data(), (int)indices.size());
}

void PolylineStyle::buildPolygon(Polygon& _polygon, std::string& _layer, Properties& _props, VboMesh& _mesh) const {
    // No-op
}
