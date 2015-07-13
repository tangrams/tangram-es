#include "polylineBatch.h"

#include "tile/mapTile.h"
#include "polylineStyle.h"
#include "tangram.h"


PolylineBatch::PolylineBatch(const PolylineStyle& _style)
    : m_style(_style) {
    m_mesh = std::make_shared<Mesh>(_style.m_vertexLayout, _style.m_drawMode);
}

bool PolylineBatch::compile() {
    if (m_mesh->numVertices() > 0) {
        m_mesh->compileVertexBuffer();
        return true;
    }
    return false;
}

void PolylineBatch::draw(const View& _view) {
    m_mesh->draw(m_style.getShaderProgram());
}

void PolylineBatch::add(const Feature& _feature, const StyleParamMap& _styleParams, const MapTile& _tile) {

    if (_feature.geometryType != GeometryType::lines) {
        return;
    }

    PolylineStyle::StyleParams params(m_style.parseParamMap(_styleParams));

    for (auto& line : _feature.lines) {
        buildLine(line, _feature.props, params, _tile);
    }
}

void PolylineBatch::buildLine(const Line& _line, const Properties& _props,
                              const PolylineStyle::StyleParams& _params, const MapTile& _tile) {

    std::vector<PosNormEnormColVertex> vertices;

    GLuint abgr = _params.color;

    if (Tangram::getDebugFlag(Tangram::DebugFlags::proxy_colors)) {
        abgr = abgr << (_tile.getID().z % 6);
    }

    GLfloat layer = _props.getNumeric("sort_key") + _params.order;
    float halfWidth = _params.width * .5f;

    PolyLineBuilder builder {
        [&](const glm::vec3& coord, const glm::vec2& normal, const glm::vec2& uv) {
            vertices.push_back({ coord, uv, normal, halfWidth, abgr, layer });
        },
        PolyLineOptions(_params.cap, _params.join)
    };

    Builders::buildPolyLine(_line, builder);

    if (_params.outlineOn) {

        GLuint abgrOutline = _params.outlineColor;
        halfWidth += _params.outlineWidth * .5f;

        if (_params.outlineCap != _params.cap || _params.outlineJoin != _params.join) {
            // need to re-triangulate with different cap and/or join
            builder.options.cap = _params.outlineCap;
            builder.options.join = _params.outlineJoin;
            Builders::buildPolyLine(_line, builder);
        } else {
            // re-use indices from original line
            size_t oldSize = builder.indices.size();
            size_t offset = vertices.size();
            builder.indices.reserve(2 * oldSize);

            for(size_t i = 0; i < oldSize; i++) {
                 builder.indices.push_back(offset + builder.indices[i]);
            }
            for (size_t i = 0; i < offset; i++) {
                const auto& v = vertices[i];
                vertices.push_back({ v.pos, v.texcoord, v.enorm, halfWidth, abgrOutline, layer - 1.f });
            }
        }
    }

    m_mesh->addVertices(std::move(vertices), std::move(builder.indices));
}
