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
    
    for (const std::string& layer : m_layers) {
        
        if (!_jsonRoot.isMember(layer.c_str())) {
            continue;
        }
        
        Json::Value layerFeatures = _jsonRoot[layer.c_str()]["features"];
        for (auto& feature : layerFeatures) {
            
            Json::Value props = feature["properties"];
            props["layer"] = layer;
            Json::Value geometry = feature["geometry"];
            Json::Value coords = geometry["coordinates"];
            std::string geometryType = geometry["type"].asString();
            
            //TODO: For all these JsonExtraction and build calls 
            //      make this a vector<*glm::vec3> for better std::vector operations

            if (geometryType.compare("Point") == 0) {
                
                glm::vec3 point;
                JsonExtractor::extractPoint(coords, point, _mapProjection, _tile.getOrigin());
                buildPoint(point, props, *mesh);
                
            } else if (geometryType.compare("MultiPoint") == 0) {
                
                for (auto& pointCoords : coords) {
                    glm::vec3 point;
                    JsonExtractor::extractPoint(pointCoords, point, _mapProjection, _tile.getOrigin());
                    buildPoint(point, props, *mesh);
                }
                
            } else if (geometryType.compare("LineString") == 0) {
                
                std::vector<glm::vec3> line;
                JsonExtractor::extractLine(coords, line, _mapProjection, _tile.getOrigin());
                buildLine(line, props, *mesh);
                
            } else if (geometryType.compare("MultiLineString") == 0) {
                
                for (auto& lineCoords: coords) {
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
                
                for (auto& polygonCoords : coords) {
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

    // TODO: Ideally this would be in the same location as the struct that it basically describes
    m_vertexLayout = std::shared_ptr<VertexLayout>(new VertexLayout({
        {"a_position", 3, GL_FLOAT, false, 0},
        {"a_normal", 3, GL_FLOAT, false, 0},
        {"a_color", 4, GL_UNSIGNED_BYTE, true, 0}
    }));
    
}

void PolygonStyle::constructShaderProgram() {
    
    // TODO: Load shader sources from file
    
    std::string vertShaderSrcStr =
        "#ifdef GL_ES\n"
        "precision mediump float;\n"
        "#endif\n"
        "uniform mat4 u_modelViewProj;\n"
        "uniform vec4 u_lightDirection;\n"
        "attribute vec4 a_position;\n"
        "attribute vec4 a_normal;\n"
        "attribute vec4 a_color;\n"
        "varying vec4 v_color;\n"
        "void main() {\n"
        "  float lit = dot(normalize(u_lightDirection), normalize(a_normal));\n"
        "  v_color = a_color;\n"
        "  v_color.rgb *= clamp(lit * 1.5, 0.5, 1.5);\n"
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
    m_shaderProgram->setUniformf("u_lightDirection", -1.0, -1.0, 1.0, 0.0);
}

void PolygonStyle::buildPoint(glm::vec3& _point, Json::Value& _props, VboMesh& _mesh) {
    // No-op
}

void PolygonStyle::buildLine(std::vector<glm::vec3>& _line, Json::Value& _props, VboMesh& _mesh) {
    // No-op
}

void PolygonStyle::buildPolygon(std::vector<glm::vec3>& _polygon, std::vector<int>& _sizes, Json::Value& _props, VboMesh& _mesh) {
    
    std::vector<PosNormColVertex> vertices;
    std::vector<GLushort> indices;
    
    std::vector<glm::vec3> points;
    std::vector<glm::vec3> normals;

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
    
    float height = 0;
    float minHeight = 0;
    
    if (_props.isMember("height")) {
        height = _props["height"].asFloat();
    }
    if (_props.isMember("min_height")) {
        minHeight = _props["min_height"].asFloat();
    }
    
    if (minHeight != height) {
        for (auto& pt : _polygon) {
            pt.z = height;
        }
        GeometryHandler::buildPolygonExtrusion(_polygon, _sizes, minHeight, points, normals, indices);
    }
    
    GeometryHandler::buildPolygon(_polygon, _sizes, points, normals, indices);
    
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
