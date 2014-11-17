#include "style.h"
#include "util/geometryHandler.h"

/*
 * Style Class Methods
 */

Style::Style(std::string _name, GLenum _drawMode) : m_name(_name), m_drawMode(_drawMode) {
}

Style::~Style() {
    m_layers.clear();
}

void Style::addLayers(std::vector<std::string> _layers) {
    m_layers.insert(_layers.cbegin(), _layers.cend());
}

void Style::addData(TileData& _data, MapTile& _tile, const MapProjection& _mapProjection) {
    
    VboMesh* mesh = new VboMesh(m_vertexLayout, m_drawMode);
    
    for (auto& layer : _data.layers) {
        
        if (m_layers.find(layer.name) == m_layers.end()) {
            continue;
        }
        
        for (auto& feature : layer.features) {

            switch (feature.geometryType) {
                case GeometryType::POINTS:
                    // Build points
                    for (auto& point : feature.points) {
                        buildPoint(point, layer.name, feature.props, *mesh);
                    }
                    break;
                case GeometryType::LINES:
                    // Build lines
                    for (auto& line : feature.lines) {
                        buildLine(line, layer.name, feature.props, *mesh);
                    }
                    break;
                case GeometryType::POLYGONS:
                    // Build polygons
                    for (auto& polygon : feature.polygons) {
                        buildPolygon(polygon, layer.name, feature.props, *mesh);
                    }
                    break;
                default:
                    break;
            }
        }
    }
    
    _tile.addGeometry(*this, std::unique_ptr<VboMesh>(mesh));
    
}

/*
 * Polygon Style Class Methods
 */

PolygonStyle::PolygonStyle(std::string _name, GLenum _drawMode) : Style(_name, _drawMode) {

    constructVertexLayout();
    constructShaderProgram();
    
}

void PolygonStyle::constructVertexLayout() {

    // TODO: Ideally this would be in the same location as the struct that it basically describes
    m_vertexLayout = std::shared_ptr<VertexLayout>(new VertexLayout({
        {"a_position", 3, GL_FLOAT, false, 0},
        {"a_normal", 3, GL_FLOAT, false, 0},
        {"a_color", 4, GL_UNSIGNED_BYTE, true, 0}
    }));
    
}

void PolygonStyle::constructShaderProgram() {
    
    std::string vertShaderSrcStr = stringFromResource("polygon.vs");
    
    std::string fragShaderSrcStr = stringFromResource("polygon.fs");
    
    m_shaderProgram = std::make_shared<ShaderProgram>();
    m_shaderProgram->buildFromSourceStrings(fragShaderSrcStr, vertShaderSrcStr);
    
}

void PolygonStyle::setup() {
    m_shaderProgram->setUniformf("u_lightDirection", -1.0, -1.0, 1.0, 0.0);
}

void PolygonStyle::buildPoint(Point& _point, std::string& _layer, Properties& _props, VboMesh& _mesh) {
    // No-op
}

void PolygonStyle::buildLine(Line& _line, std::string& _layer, Properties& _props, VboMesh& _mesh) {
    std::vector<PosNormColVertex> vertices;
    std::vector<GLushort> indices;
    std::vector<glm::vec3> points;
    
    GLuint abgr = 0xff969696; // Default road color
    float halfWidth = 0.03;
    
    GeometryHandler::buildPolyLine(_line, halfWidth, points, indices);
    
    for (int i = 0; i < points.size(); i++) {
        glm::vec3 p = points[i];
        glm::vec3 n = glm::vec3(0.0f, 0.0f, 1.0f);
        vertices.push_back({ p.x, p.y, p.z, n.x, n.y, n.z, abgr });
    }
    
    // Make sure indices get correctly offset
    int vertOffset = _mesh.numVertices();
    logMsg("Adding to prev Mesh : %i \n",vertOffset);
    for (auto& ind : indices) {
        ind += vertOffset;
    }
    
    _mesh.addVertices((GLbyte*)vertices.data(), vertices.size());
    _mesh.addIndices(indices.data(), indices.size());
}

void PolygonStyle::buildPolygon(Polygon& _polygon, std::string& _layer, Properties& _props, VboMesh& _mesh) {
    
    std::vector<PosNormColVertex> vertices;
    std::vector<GLushort> indices;
    std::vector<glm::vec3> points;
    std::vector<glm::vec3> normals;

    GLuint abgr = 0xffaaaaaa; // Default color
    
    if (_layer.compare("buildings") == 0) {
        abgr = 0xffcedcde;
    } else if (_layer.compare("water") == 0) {
        abgr = 0xff917d1a;
    } else if (_layer.compare("roads") == 0) {
        abgr = 0xff969696;
    } else if (_layer.compare("earth") == 0) {
        abgr = 0xff669171;
    } else if (_layer.compare("landuse") == 0) {
        abgr = 0xff507480;
    }
    
    float height = _props.numericProps["height"]; // Inits to zero if not present in data
    float minHeight = _props.numericProps["min_height"]; // Inits to zero if not present in data
    
    if (minHeight != height) {
        for (auto& line : _polygon) {
            for (auto& point : line) {
                point.z = height;
            }
        }
        GeometryHandler::buildPolygonExtrusion(_polygon, minHeight, points, normals, indices);
    }
    
    GeometryHandler::buildPolygon(_polygon, points, normals, indices);
    
    for (int i = 0; i < points.size(); i++) {
        glm::vec3 p = points[i];
        glm::vec3 n = normals[i];
        vertices.push_back({ p.x, p.y, p.z, n.x, n.y, n.z, abgr });
    }

    // Make sure indices get correctly offset
    int vertOffset = _mesh.numVertices();
    for (auto& ind : indices) {
        ind += vertOffset;
    }
    
    _mesh.addVertices((GLbyte*)vertices.data(), vertices.size());
    _mesh.addIndices(indices.data(), indices.size());
    
}
