#include "polygonStyle.h"

#include "tangram.h"
#include "util/builders.h"
#include "util/shaderProgram.h"
#include "tile/mapTile.h"

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

StyleBatch* PolygonStyle::newBatch() const {
    return new PolygonBatch(*this);
};

void PolygonBatch::add(const Feature& _feature, const StyleParamMap& _styleParams, const MapTile& _tile) {

    StyleParams params;

    if (_styleParams.find("order") != _styleParams.end()) {
        params.order = std::stof(_styleParams.at("order"));
    }
    if (_styleParams.find("color") != _styleParams.end()) {
        params.color = Style::parseColorProp(_styleParams.at("color"));
    }

    switch (_feature.geometryType) {
        case GeometryType::lines:
            for (auto& line : _feature.lines) {
                buildLine(line, _feature.props, params, _tile);
            }
            break;
        case GeometryType::polygons:
            for (auto& polygon : _feature.polygons) {
                buildPolygon(polygon, _feature.props, params, _tile);
            }
            break;
        default:
            break;
    }
}

void PolygonBatch::buildLine(const Line& _line, const Properties& _props,
                             const StyleParams& _params, const MapTile& _tile) {

    std::vector<PosNormColVertex> vertices;

    GLuint abgr = _params.color;
    GLfloat layer = _params.order;

    PolyLineBuilder builder = {
        [&](const glm::vec3& coord, const glm::vec2& normal, const glm::vec2& uv) {
            float halfWidth =  0.2f;

            glm::vec3 point(coord.x + normal.x * halfWidth, coord.y + normal.y * halfWidth, coord.z);
            vertices.push_back({ point, glm::vec3(0.0f, 0.0f, 1.0f), uv, abgr, layer });
        }
    };

    Builders::buildPolyLine(_line, builder);

    m_mesh->addVertices(std::move(vertices), std::move(builder.indices));
}

void PolygonBatch::buildPolygon(const Polygon& _polygon, const Properties& _props,
                                const StyleParams& _params, const MapTile& _tile) {

    std::vector<PosNormColVertex> vertices;

    GLuint abgr = _params.color;
    GLfloat layer = _params.order;

    if (Tangram::getDebugFlag(Tangram::DebugFlags::proxy_colors)) {
        abgr = abgr << (_tile.getID().z % 6);
    }

    float height = _props.getNumeric("height");
    float minHeight = _props.getNumeric("min_height");

    PolygonBuilder builder = {
        [&](const glm::vec3& coord, const glm::vec3& normal, const glm::vec2& uv){
            vertices.push_back({ { coord.x, coord.y, height }, normal, uv, abgr, layer });
        },
        [&](size_t sizeHint){ vertices.reserve(sizeHint); },
    };

    Builders::buildPolygon(_polygon, builder);

    if (minHeight != height) {
        builder.addVertex = [&](const glm::vec3& coord, const glm::vec3& normal, const glm::vec2& uv) {
            vertices.push_back({ coord, normal, uv, abgr, layer });
        };

        Builders::buildPolygonExtrusion(_polygon, minHeight, height, builder);
    }


    m_mesh->addVertices(std::move(vertices), std::move(builder.indices));
}
