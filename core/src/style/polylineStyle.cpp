#include "polylineStyle.h"

#include "tangram.h"
#include "util/shaderProgram.h"

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


void PolylineStyle::build(Batch& _batch, const Feature& _feature,
                          const StyleParamMap& _styleParams,
                          const MapTile& _tile) const {

    StyleParams params;

    if (_feature.geometryType != GeometryType::lines) {
        return;
    }

    if(_styleParams.find("order") != _styleParams.end()) {
        params.order = std::stof(_styleParams.at("order"));
    }

    if(_styleParams.find("color") != _styleParams.end()) {
        params.color = parseColorProp(_styleParams.at("color"));
    }

    if(_styleParams.find("width") != _styleParams.end()) {
        params.width = std::stof(_styleParams.at("width"));
    }

    if(_styleParams.find("cap") != _styleParams.end()) {
        std::string capStr = _styleParams.at("cap");
        if(capStr == "butt") { params.cap = CapTypes::butt; }
        else if(capStr == "square") { params.cap = CapTypes::square; }
        else if(capStr == "round") { params.cap = CapTypes::round; }
    }

    if(_styleParams.find("join") != _styleParams.end()) {
        std::string joinStr = _styleParams.at("join");
        if(joinStr == "bevel") { params.join = JoinTypes::bevel; }
        else if(joinStr == "miter") { params.join = JoinTypes::miter; }
        else if(joinStr == "round") { params.join = JoinTypes::round; }
    }

    if(_styleParams.find("outline:width") != _styleParams.end()) {
        params.outlineOn = true;
        params.outlineWidth = std::stof(_styleParams.at("outline:width"));
    }

    if(_styleParams.find("outline:color") != _styleParams.end()) {
        params.outlineColor =  parseColorProp(_styleParams.at("outline:color"));
    }

    if(_styleParams.find("outline:cap") != _styleParams.end()) {
        params.outlineOn = true;
        std::string capStr = _styleParams.at("outline:cap");
        if(capStr == "butt") { params.outlineCap = CapTypes::butt; }
        else if(capStr == "square") { params.outlineCap = CapTypes::square; }
        else if(capStr == "round") { params.outlineCap = CapTypes::round; }
    }

    if( _styleParams.find("outline:join") != _styleParams.end()) {
        params.outlineOn = true;
        std::string joinStr = _styleParams.at("outline:join");
        if(joinStr == "bevel") { params.outlineJoin = JoinTypes::bevel; }
        else if(joinStr == "miter") { params.outlineJoin = JoinTypes::miter; }
        else if(joinStr == "round") { params.outlineJoin = JoinTypes::round; }
    }

    auto& batch = static_cast<PolylineBatch&>(_batch);

    for (auto& line : _feature.lines) {
        buildLine(batch, line, params, _feature.props, _tile);
    }
}

void PolylineStyle::buildLine(PolylineBatch& _batch, const Line& _line, const StyleParams& _params,
                              const Properties& _props, const MapTile& _tile) const {

    std::vector<PosNormEnormColVertex> vertices;

    GLuint abgr = _params.color;

    if (Tangram::getDebugFlag(Tangram::DebugFlags::proxy_colors)) {
        abgr = abgr << (int(_props.getNumeric("zoom")) % 6);
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

    _batch.m_mesh->addVertices(std::move(vertices), std::move(builder.indices));
}
