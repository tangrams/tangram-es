#include "polygonStyle.h"
#include "util/builders.h"
#include "roadLayers.h"
#include "tangram.h"

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

void* PolygonStyle::parseStyleParams(const std::string& _layerNameID, const StyleParamMap& _styleParamMap) {

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

    {
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        m_styleParamCache.emplace(_layerNameID, params);
    }

    return static_cast<void*>(params);
}

void PolygonStyle::buildPoint(Point& _point, void* _styleParam, Properties& _props, VboMesh& _mesh) const {
    // No-op
}

void PolygonStyle::buildLine(Line& _line, void* _styleParam, Properties& _props, VboMesh& _mesh) const {
    std::vector<PosNormColVertex> vertices;
    std::vector<int> indices;
    std::vector<glm::vec3> points;
    std::vector<glm::vec2> texcoords;

    PolyLineOutput output = { points, indices, Builders::NO_SCALING_VECS, texcoords };

    GLuint abgr = 0xff969696; // Default road color

    Builders::buildPolyLine(_line, PolyLineOptions(), output);

    for (size_t i = 0; i < points.size(); i++) {
        vertices.push_back({ points[i], glm::vec3(0.0f, 0.0f, 1.0f), texcoords[i], abgr, 0.0f });
    }

    auto& mesh = static_cast<PolygonStyle::Mesh&>(_mesh);
    mesh.addVertices(std::move(vertices),std::move(indices));
}

void PolygonStyle::buildPolygon(Polygon& _polygon, void* _styleParam, Properties& _props, VboMesh& _mesh) const {

    std::vector<PosNormColVertex> vertices;
    std::vector<int> indices;
    std::vector<glm::vec3> points;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texcoords;

    PolygonOutput output = { points, indices, normals, texcoords };

    StyleParams* params = static_cast<StyleParams*>(_styleParam);

    GLuint abgr = params->color;
    GLfloat layer = params->order;

    if (Tangram::getDebugFlag(Tangram::DebugFlags::PROXY_COLORS)) {
        abgr = abgr << (int(_props.numericProps[TAG_KEY_ZOOM]) % 6);
    }

    float height = _props.numericProps[TAG_KEY_HEIGHT]; // Inits to zero if not present in data
    float minHeight = _props.numericProps[TAG_KEY_MIN_HEIGHT]; // Inits to zero if not present in data

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
        vertices.push_back({ points[i], normals[i], texcoords[i], abgr, layer });
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
