#include "polygonStyle.h"
#include "util/builders.h"
#include "roadLayers.h"
#include "csscolorparser.hpp"
#include "tangram.h"

using namespace CSSColorParser;

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
        {"a_color", 4, GL_UNSIGNED_BYTE, true, 0},
        {"a_layer", 1, GL_FLOAT, false, 0}
    }));

}

void PolygonStyle::constructShaderProgram() {

    std::string vertShaderSrcStr = stringFromResource("polygon.vs");
    std::string fragShaderSrcStr = stringFromResource("polygon.fs");

    m_shaderProgram = std::make_shared<ShaderProgram>();
    m_shaderProgram->setSourceStrings(fragShaderSrcStr, vertShaderSrcStr);
}

void* PolygonStyle::parseStyleParams(StyleParamMap& _styleParamMap) const {
    StyleParams* params = new StyleParams();
    if(_styleParamMap.find("order") != _styleParamMap.end()) {
        params->order = std::stof(_styleParamMap.at("order"));
    }
    if(_styleParamMap.find("color") != _styleParamMap.end()) {
        Color c = parse(_styleParamMap.at("color"));
        params->color = c.getInt();
    }
    return static_cast<void*>(params);
}

void PolygonStyle::buildPoint(Point& _point, void* _styleParams, Properties& _props, VboMesh& _mesh) const {
    // No-op
}

void PolygonStyle::buildLine(Line& _line, void* _styleParams, Properties& _props, VboMesh& _mesh) const {
    std::vector<PosNormColVertex> vertices;
    std::vector<int> indices;
    std::vector<glm::vec3> points;
    std::vector<glm::vec2> texcoords;

    PolyLineOutput output = { points, indices, Builders::NO_SCALING_VECS, texcoords };

    GLuint abgr = 0xff969696; // Default road color

    Builders::buildPolyLine(_line, PolyLineOptions(), output);

    for (size_t i = 0; i < points.size(); i++) {
        glm::vec3 p = points[i];
        glm::vec3 n = glm::vec3(0.0f, 0.0f, 1.0f);
        glm::vec2 u = texcoords[i];
        vertices.push_back({ p.x, p.y, p.z, n.x, n.y, n.z, u.x, u.y, abgr });
    }

    auto& mesh = static_cast<PolygonStyle::Mesh&>(_mesh);
    mesh.addVertices(std::move(vertices),std::move(indices));
}

void PolygonStyle::buildPolygon(Polygon& _polygon, void* _styleParams, Properties& _props, VboMesh& _mesh) const {

    std::vector<PosNormColVertex> vertices;
    std::vector<int> indices;
    std::vector<glm::vec3> points;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texcoords;
    
    StyleParams* params = static_cast<StyleParams*>(_styleParams);

    PolygonOutput output = { points, indices, normals, texcoords };

    GLuint abgr = params->color;
    GLfloat layer = params->order;

    if (Tangram::getDebugFlag(Tangram::DebugFlags::PROXY_COLORS)) {
        abgr = abgr << (int(_props.numericProps["zoom"]) % 6);
    }

    float height = _props.numericProps["height"]; // Inits to zero if not present in data
    float minHeight = _props.numericProps["min_height"]; // Inits to zero if not present in data

    if (minHeight != height) {
        for (auto& line : _polygon) {
            for (auto& point : line) {
                point.z = height;
            }
        }
        Builders::buildPolygonExtrusion(_polygon, minHeight, output);
    }

    Builders::buildPolygon(_polygon, output);

    for (size_t i = 0; i < points.size(); i++) {
        glm::vec3 p = points[i];
        glm::vec3 n = normals[i];
        glm::vec2 u = texcoords[i];
        vertices.push_back({ p.x, p.y, p.z, n.x, n.y, n.z, u.x, u.y, abgr, layer });
    }

    // Outlines for water polygons
    /*
    if (_layer == "water") {
        abgr = 0xfff2cc6c;
        size_t outlineStart = points.size();
        PolyLineOutput lineOutput = { points, indices, Builders::NO_SCALING_VECS, texcoords };
        PolyLineOptions outlineOptions = { CapTypes::ROUND, JoinTypes::ROUND, 0.02f };
        Builders::buildOutline(_polygon[0], outlineOptions, lineOutput);
        glm::vec3 normal(0.f, 0.f, 1.f); // The outline builder doesn't produce normals, so we'll add those now
        normals.insert(normals.end(), points.size() - normals.size(), normal);
        for (size_t i = outlineStart; i < points.size(); i++) {
            glm::vec3& p = points[i];
            glm::vec3& n = normals[i];
            glm::vec2& u = texcoords[i];
            vertices.push_back({ p.x, p.y, p.z + .02f, n.x, n.y, n.z, u.x, u.y, abgr });
        }
    }
    */
    
    auto& mesh = static_cast<PolygonStyle::Mesh&>(_mesh);
    mesh.addVertices(std::move(vertices), std::move(indices));
}
