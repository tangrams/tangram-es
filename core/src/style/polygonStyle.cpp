#include "polygonStyle.h"

#include "tangram.h"
#include "util/builders.h"
#include "util/shaderProgram.h"

PolygonStyle::PolygonStyle(std::string _name, GLenum _drawMode) : Style(_name, _drawMode) {
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

    m_shaderProgram->setSourceStrings(fragShaderSrcStr, vertShaderSrcStr);
}

void PolygonStyle::parseStyleParams(const StyleParamMap& _styleParamMap, StyleParams& _styleParams) const {

    if(_styleParamMap.find("order") != _styleParamMap.end()) {
        _styleParams.order = std::stof(_styleParamMap.at("order"));
    }
    if(_styleParamMap.find("color") != _styleParamMap.end()) {
        _styleParams.color = parseColorProp(_styleParamMap.at("color"));
    }
}

void PolygonStyle::buildPoint(Point& _point, const StyleParamMap& _styleParamMap, Properties& _props, VboMesh& _mesh) const {
    // No-op
}

void PolygonStyle::buildLine(Line& _line, const StyleParamMap& _styleParamMap, Properties& _props, VboMesh& _mesh) const {
    std::vector<PosNormColVertex> vertices;

    StyleParams params;
    parseStyleParams(_styleParamMap, params);

    GLuint abgr = params.color;
    GLfloat layer = params.order;

    PolyLineBuilder builder = {
        [&](const glm::vec3& coord, const glm::vec2& normal, const glm::vec2& uv) {
            float halfWidth =  0.2f;

            glm::vec3 point(coord.x + normal.x * halfWidth, coord.y + normal.y * halfWidth, coord.z);
            vertices.push_back({ point, glm::vec3(0.0f, 0.0f, 1.0f), uv, abgr, layer });
        }
    };

    Builders::buildPolyLine(_line, builder);

    auto& mesh = static_cast<PolygonStyle::Mesh&>(_mesh);
    mesh.addVertices(std::move(vertices), std::move(builder.indices));
}

void PolygonStyle::buildPolygon(Polygon& _polygon, const StyleParamMap& _styleParamMap, Properties& _props, VboMesh& _mesh) const {

    std::vector<PosNormColVertex> vertices;

    StyleParams params;
    parseStyleParams(_styleParamMap, params);

    GLuint abgr = params.color;
    GLfloat layer = params.order;

    if (Tangram::getDebugFlag(Tangram::DebugFlags::proxy_colors)) {
        abgr = abgr << (int(_props.numericProps["zoom"]) % 6);
    }

    float height = _props.numericProps["height"]; // Inits to zero if not present in data
    float minHeight = _props.numericProps["min_height"]; // Inits to zero if not present in data

    PolygonBuilder builder = {
        [&](const glm::vec3& coord, const glm::vec3& normal, const glm::vec2& uv){
            vertices.push_back({ coord, normal, uv, abgr, layer });
        },
        [&](size_t sizeHint){ vertices.reserve(sizeHint); }
    };

    if (minHeight != height) {
        for (auto& line : _polygon) {
            for (auto& point : line) {
                point.z = height;
            }
        }
        Builders::buildPolygonExtrusion(_polygon, minHeight, builder);
    }

    Builders::buildPolygon(_polygon, builder);

    auto& mesh = static_cast<PolygonStyle::Mesh&>(_mesh);
    mesh.addVertices(std::move(vertices), std::move(builder.indices));
}
