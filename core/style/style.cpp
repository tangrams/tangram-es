#include "style.h"
#include "util/geometryHandler.h"
#include "util/jsonExtractor.h"

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

void Style::addData(const Json::Value& _jsonRoot, MapTile& _tile, const MapProjection& _mapProjection) {
    
    VboMesh* mesh = new VboMesh(m_vertexLayout, m_drawMode);
    
    for (std::string layer : m_layers) {
        
        if (!_jsonRoot.isMember(layer.c_str())) {
            continue;
        }
        
        Json::Value layerFeatures = _jsonRoot[layer.c_str()]["features"];
        for (auto feature : layerFeatures) {
            
            Json::Value props = feature["properties"];
            props["layer"] = layer;
            Json::Value geometry = feature["geometry"];
            Json::Value coords = geometry["coordinates"];
            std::string geometryType = geometry["type"].asString();
            
            if (geometryType.compare("Point") == 0) {
                
                glm::vec3 point;
                JsonExtractor::extractPoint(coords, point, _mapProjection, _tile.getOrigin());
                buildPoint(point, props, *mesh);
                
            } else if (geometryType.compare("MultiPoint") == 0) {
                
                for (auto pointCoords : coords) {
                    glm::vec3 point;
                    JsonExtractor::extractPoint(pointCoords, point, _mapProjection, _tile.getOrigin());
                    buildPoint(point, props, *mesh);
                }
                
            } else if (geometryType.compare("LineString") == 0) {
                
                std::vector<glm::vec3> line;
                JsonExtractor::extractLine(coords, line, _mapProjection, _tile.getOrigin());
                buildLine(line, props, *mesh);
                
            } else if (geometryType.compare("MultiLineString") == 0) {
                
                for (auto lineCoords: coords) {
                    std::vector<glm::vec3> line;
                    JsonExtractor::extractLine(lineCoords, line, _mapProjection, _tile.getOrigin());
                    buildLine(line, props, *mesh);
                }
                
            } else if (geometryType.compare("Polygon") == 0) {
                
                std::vector<glm::vec3> polygon;
                std::vector<int> sizes;
                JsonExtractor::extractPoly(coords, polygon, sizes, _mapProjection, _tile.getOrigin());
                buildPolygon(polygon, sizes, props, *mesh);
                
            } else if (geometryType.compare("MultiPolygon") == 0) {
                
                for (auto polygonCoords : coords) {
                    std::vector<glm::vec3> polygon;
                    std::vector<int> sizes;
                    JsonExtractor::extractPoly(polygonCoords, polygon, sizes, _mapProjection, _tile.getOrigin());
                    buildPolygon(polygon, sizes, props, *mesh);
                }
                
            } else if (geometryType.compare("GeometryCollection") == 0) {
                logMsg("Friendly reminder that GeometryCollection objects still need to be handled\n");
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

    m_vertexLayout = std::shared_ptr<VertexLayout>(new VertexLayout({
        {"a_position", 3, GL_FLOAT, false, 0},
        {"a_normal", 3, GL_FLOAT, false, 0},
        {"a_color", 4, GL_UNSIGNED_BYTE, true, 0}
    }));
    
}

void PolygonStyle::constructShaderProgram() {
    
    std::string vertShaderSrcStr =
        "#ifdef GL_ES\n"
        "precision mediump float;\n"
        "#endif\n"
        "uniform mat4 u_modelViewProj;\n"
        "attribute vec4 a_position;\n"
        "attribute vec4 a_color;\n"
        "varying vec4 v_color;\n"
        "void main() {\n"
        "  v_color = a_color;\n"
        "  gl_Position = u_modelViewProj * a_position;\n"
        "}\n";
    
    std::string fragShaderSrcStr =
        "#ifdef GL_ES\n"
        "precision mediump float;\n"
        "#endif\n"
        "varying vec4 v_color;\n"
        "void main(void) {\n"
        "  gl_FragColor = v_color;\n"
        "}\n";
    
    m_shaderProgram = std::make_shared<ShaderProgram>();
    m_shaderProgram->buildFromSourceStrings(fragShaderSrcStr, vertShaderSrcStr);
    
}

void PolygonStyle::setup() {
}

void PolygonStyle::buildPoint(const glm::vec3& _point, const Json::Value& _props, VboMesh& _mesh) {
    // No-op
}

void PolygonStyle::buildLine(const std::vector<glm::vec3>& _line, const Json::Value& _props, VboMesh& _mesh) {
    // No-op
}

void PolygonStyle::buildPolygon(const std::vector<glm::vec3>& _polygon, const std::vector<int>& _sizes, const Json::Value& _props, VboMesh& _mesh) {
    
    GLuint abgr = 0xffaaaaaa; // Default color
    
    std::string layer = _props["layer"].asString();
    if (layer.compare("buildings") == 0) {
        abgr = 0xffcedcde;
    } else if (layer.compare("water") == 0) {
        abgr = 0xff917d1a;
    } else if (layer.compare("roads") == 0) {
        abgr = 0xff969696;
    } else if (layer.compare("earth") == 0) {
        abgr = 0xff669171;
    } else if (layer.compare("landuse") == 0) {
        abgr = 0xff507480;
    }
    
    // Make sure indices get correctly offset here
    int vertOffset = static_cast<int>(m_points.size());
    int indOffset = static_cast<int>(m_indices.size());
    GeometryHandler::buildPolygon(_polygon, _sizes, m_points, m_normals, m_indices);
    
    for (int i = vertOffset; i < m_points.size(); i++) {
        glm::vec3 p = m_points[i];
        glm::vec3 n = m_normals[i];
        m_vertices.push_back({ p.x, p.y, p.z, n.x, n.y, n.z, abgr });
    }
    
    _mesh.addVertices((GLbyte*)&m_vertices[vertOffset], static_cast<int>(m_points.size() - vertOffset));
    _mesh.addIndices(&m_indices[indOffset], static_cast<int>(m_indices.size() - indOffset));
    
}

void PolygonStyle::addData(const Json::Value &_jsonRoot, MapTile &_tile, const MapProjection &_mapProjection) {
    
    Style::addData(_jsonRoot, _tile, _mapProjection);
    
    m_vertices.clear();
    m_indices.clear();
    m_points.clear();
    m_normals.clear();
}
