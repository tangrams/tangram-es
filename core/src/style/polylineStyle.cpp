#include "polylineStyle.h"

#include "tangram.h"
#include "gl/shaderProgram.h"
#include "tile/tile.h"

namespace Tangram {

PolylineStyle::PolylineStyle(std::string _name, GLenum _drawMode) : Style(_name, _drawMode) {
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

    m_shaderProgram->setSourceStrings(fragShaderSrcStr, vertShaderSrcStr);
}

PolylineStyle::Parameters PolylineStyle::parseStyleParams(const StyleParamMap& _styleParamMap) const {

    Parameters params;

    auto it = _styleParamMap.find("order");
    if (it != _styleParamMap.end()) {
        params.order = std::stof(it->second);
    }

    if ((it = _styleParamMap.find("color")) != _styleParamMap.end()) {
        params.color = parseColorProp(it->second);
    }

    if ((it =_styleParamMap.find("width")) != _styleParamMap.end()) {
        params.width = std::stof(it->second);
    }

    if ((it = _styleParamMap.find("cap")) != _styleParamMap.end()) {
        params.cap = CapTypeFromString(it->second);
    }

    if ((it = _styleParamMap.find("join")) != _styleParamMap.end()) {
        params.join = JoinTypeFromString(it->second);
    }

    if ((it = _styleParamMap.find("outline:width")) != _styleParamMap.end()) {
        params.outlineOn = true;
        params.outlineWidth = std::stof(it->second);
    }

    if ((it = _styleParamMap.find("outline:color")) != _styleParamMap.end()) {
        params.outlineColor =  parseColorProp(it->second);
    }

    if ((it = _styleParamMap.find("outline:cap")) != _styleParamMap.end()) {
        params.outlineOn = true;
        params.outlineCap = CapTypeFromString(it->second);
    }

    if ((it = _styleParamMap.find("outline:join")) != _styleParamMap.end()) {
        params.outlineOn = true;
        params.outlineJoin = JoinTypeFromString(it->second);
    }
    return params;
}

void PolylineStyle::buildLine(const Line& _line, const StyleParamMap& _styleParamMap, const Properties& _props, VboMesh& _mesh, Tile& _tile) const {
    std::vector<PolylineVertex> vertices;

    Parameters params = parseStyleParams(_styleParamMap);
    GLuint abgr = params.color;

    if (Tangram::getDebugFlag(Tangram::DebugFlags::proxy_colors)) {
        abgr = abgr << (_tile.getID().z % 6);
    }

    GLfloat layer = _props.getNumeric("sort_key", 0) + params.order;
    float halfWidth = params.width * .5f;

    PolyLineBuilder builder {
        [&](const glm::vec3& coord, const glm::vec2& normal, const glm::vec2& uv) {
            vertices.push_back({ coord, uv, normal, halfWidth, abgr, layer });
        },
        params.cap,
        params.join
    };

    Builders::buildPolyLine(_line, builder);

    if (params.outlineOn) {

        GLuint abgrOutline = params.outlineColor;
        halfWidth += params.outlineWidth * .5f;

        if (params.outlineCap != params.cap || params.outlineJoin != params.join) {
            // need to re-triangulate with different cap and/or join
            builder.cap = params.outlineCap;
            builder.join = params.outlineJoin;
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

    auto& mesh = static_cast<PolylineStyle::Mesh&>(_mesh);
    mesh.addVertices(std::move(vertices), std::move(builder.indices));
}

}
