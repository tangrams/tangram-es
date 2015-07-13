#include "polygonBatch.h"

#include "polygonStyle.h"
#include "tangram.h"
#include "util/builders.h"
#include "tile/mapTile.h"


PolygonBatch::PolygonBatch(const PolygonStyle& _style)
    : MeshBatch(_style, std::make_shared<Mesh>(_style.m_vertexLayout, _style.m_drawMode)) {
}

void PolygonBatch::add(const Feature& _feature, const StyleParamMap& _styleParams, const MapTile& _tile) {

    PolygonStyle::StyleParams params(m_style.parseParamMap(_styleParams));

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
                             const PolygonStyle::StyleParams& _params, const MapTile& _tile) {

    std::vector<PosNormColVertex> vertices;

    PolyLineBuilder builder = {
        [&](const glm::vec3& coord, const glm::vec2& normal, const glm::vec2& uv) {
            float halfWidth =  0.2f;
            GLuint abgr = 0xff969696; // Default road color

            glm::vec3 point(coord.x + normal.x * halfWidth, coord.y + normal.y * halfWidth, coord.z);
            vertices.push_back({ point, glm::vec3(0.0f, 0.0f, 1.0f), uv, abgr, 0.0f });
        }
    };

    Builders::buildPolyLine(_line, builder);

    m_mesh->addVertices(std::move(vertices), std::move(builder.indices));
}

void PolygonBatch::buildPolygon(const Polygon& _polygon, const Properties& _props,
                                const PolygonStyle::StyleParams& _params, const MapTile& _tile) {

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
