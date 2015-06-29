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

void* PolylineStyle::parseStyleParams(const std::string& _layerNameID, const StyleParamMap& _styleParamMap) {

    if(m_styleParamCache.find(_layerNameID) != m_styleParamCache.end()) {
        return static_cast<void*>(m_styleParamCache.at(_layerNameID));
    }

    StyleParams* params = new StyleParams();

    if(_styleParamMap.find("order") != _styleParamMap.end()) {
        params->order = std::stof(_styleParamMap.at("order"));
    }

    if(_styleParamMap.find("color") != _styleParamMap.end()) {
        params->color = parseColorProp(_styleParamMap.at("color"));
    }

    if(_styleParamMap.find("width") != _styleParamMap.end()) {
        params->width = std::stof(_styleParamMap.at("width"));
    }

    if(_styleParamMap.find("cap") != _styleParamMap.end()) {
        std::string capStr = _styleParamMap.at("cap");
        if(capStr == "butt") { params->cap = CapTypes::butt; }
        else if(capStr == "square") { params->cap = CapTypes::square; }
        else if(capStr == "round") { params->cap = CapTypes::round; }
    }

    if(_styleParamMap.find("join") != _styleParamMap.end()) {
        std::string joinStr = _styleParamMap.at("join");
        if(joinStr == "bevel") { params->join = JoinTypes::bevel; }
        else if(joinStr == "miter") { params->join = JoinTypes::miter; }
        else if(joinStr == "round") { params->join = JoinTypes::round; }
    }

    if(_styleParamMap.find("outline:width") != _styleParamMap.end()) {
        params->outlineOn = true;
        params->outlineWidth = std::stof(_styleParamMap.at("outline:width"));
    }

    if(_styleParamMap.find("outline:color") != _styleParamMap.end()) {
        params->outlineColor =  parseColorProp(_styleParamMap.at("outline:color"));
    }

    if(_styleParamMap.find("outline:cap") != _styleParamMap.end()) {
        params->outlineOn = true;
        std::string capStr = _styleParamMap.at("outline:cap");
        if(capStr == "butt") { params->outlineCap = CapTypes::butt; }
        else if(capStr == "square") { params->outlineCap = CapTypes::square; }
        else if(capStr == "round") { params->outlineCap = CapTypes::round; }
    }

    if( _styleParamMap.find("outline:join") != _styleParamMap.end()) {
        params->outlineOn = true;
        std::string joinStr = _styleParamMap.at("outline:join");
        if(joinStr == "bevel") { params->outlineJoin = JoinTypes::bevel; }
        else if(joinStr == "miter") { params->outlineJoin = JoinTypes::miter; }
        else if(joinStr == "round") { params->outlineJoin = JoinTypes::round; }
    }

    {
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        m_styleParamCache.emplace(_layerNameID, params);
    }

    return static_cast<void*>(params);
}

void PolylineStyle::buildPoint(Point& _point, void* _styleParam, Properties& _props, VboMesh& _mesh) const {
    // No-op
}

void PolylineStyle::buildLine(Line& _line, void* _styleParam, Properties& _props, VboMesh& _mesh) const {
    std::vector<PosNormEnormColVertex> vertices;

    StyleParams* params = static_cast<StyleParams*>(_styleParam);
    GLuint abgr = params->color;

    if (Tangram::getDebugFlag(Tangram::DebugFlags::proxy_colors)) {
        abgr = abgr << (int(Props::getFloat(_props, TAG_KEY_ZOOM)) % 6);
    }

    GLfloat layer = Props::getFloat(_props, TAG_KEY_SORT_KEY) + params->order;
    float halfWidth = params->width * .5f;

    PolyLineBuilder builder {
        [&](const glm::vec3& coord, const glm::vec2& normal, const glm::vec2& uv) {
            vertices.push_back({ coord, uv, normal, halfWidth, abgr, layer });
        },
        PolyLineOptions(params->cap, params->join)
    };

    Builders::buildPolyLine(_line, builder);

    if (params->outlineOn) {

        GLuint abgrOutline = params->outlineColor;
        halfWidth += params->outlineWidth * .5f;

        if (params->outlineCap != params->cap || params->outlineJoin != params->join) {
            // need to re-triangulate with different cap and/or join
            builder.options.cap = params->outlineCap;
            builder.options.join = params->outlineJoin;
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

void PolylineStyle::buildPolygon(Polygon& _polygon, void* _styleParam, Properties& _props, VboMesh& _mesh) const {
    // No-op
}
