#include "style.h"
#include "platform.h"
#include "util/geometryHandler.h"

/*
 * Style Class Methods
 */
Style::Style(std::string _geomType, GLenum _drawMode) : m_geomType(_geomType), m_styleName(_geomType), m_drawMode(_drawMode) {
}

Style::Style(std::string _geomType, std::string _styleName, GLenum _drawMode) : m_geomType(_geomType), m_drawMode(_drawMode) {
}

void Style::setFragShaderSrc(std::string _fragShaderSrcStr) {
    m_fragShaderSrcStr = std::move(_fragShaderSrcStr);
}

void Style::setVertShaderSrc(std::string _vertShaderSrcStr) {
    m_vertShaderSrcStr = std::move(_vertShaderSrcStr);
}

std::shared_ptr<ShaderProgram> Style::getShaderProgram() const {
    return m_shaderProgram;
}

void Style::updateLayers(std::vector<std::pair<std::string, glm::vec4>> _newLayers) {
    for(auto& newLayer : _newLayers) {
        if(m_layerColorMap.find(newLayer.first) == m_layerColorMap.end()) {
            m_layerColorMap.emplace(std::make_pair(newLayer.first, newLayer.second));
        }
        else {
            m_layerColorMap[newLayer.first] = newLayer.second;
        }
    }
}

/*
 * Polygon Style Class Methods
 */
PolygonStyle::PolygonStyle(std::string _geomType, GLenum _drawMode) : Style(_geomType, _drawMode) {
    
    // Pass in a fileName or a url to construct shader strings from
    m_vertShaderSrcStr =
        "#ifdef GL_ES\n"
        "precision mediump float;\n"
        "#endif\n"
        "attribute vec4 a_position;\n"
        "attribute vec4 a_color;\n"
        "varying vec4 v_color;\n"
        "void main() {\n"
        "  v_color = a_color;\n"
        "  gl_Position = a_position;\n"
        "}\n";

    m_fragShaderSrcStr =
        "#ifdef GL_ES\n"
        "precision mediump float;\n"
        "#endif\n"
        "varying vec4 v_color;\n"
        "void main(void) {\n"
        "  gl_FragColor = v_color;\n"
        "}\n";
    constructVertexLayout();
    constructShaderProgram();
}

PolygonStyle::PolygonStyle(std::string _geomType, std::string _styleName, GLenum _drawMode) : Style(_geomType, _styleName, _drawMode) {
    
    // Pass in a fileName or a url to construct shader strings from
    m_vertShaderSrcStr =
        "#ifdef GL_ES\n"
        "precision mediump float;\n"
        "#endif\n"
        "attribute vec4 a_position;\n"
        "attribute vec4 a_color;\n"
        "varying vec4 v_color;\n"
        "void main() {\n"
        "  v_color = a_color;\n"
        "  gl_Position = a_position;\n"
        "}\n";

    m_fragShaderSrcStr =
        "#ifdef GL_ES\n"
        "precision mediump float;\n"
        "#endif\n"
        "varying vec4 v_color;\n"
        "void main(void) {\n"
        "  gl_FragColor = v_color;\n"
        "}\n";
    constructVertexLayout();
    constructShaderProgram();
}

void PolygonStyle::constructVertexLayout() {
    // fixed per style... this is basic position, normal color
    m_vertexLayout = std::shared_ptr<VertexLayout>(new VertexLayout({
        {"a_position", 3, GL_FLOAT, false, 0},
        {"a_normal", 3, GL_FLOAT, false, 0},
        {"a_color", 4, GL_UNSIGNED_BYTE, true, 0}
    }));
}

void PolygonStyle::constructShaderProgram() {
    m_shaderProgram = std::shared_ptr<ShaderProgram>(new ShaderProgram());
    m_shaderProgram->buildFromSourceStrings(m_fragShaderSrcStr, m_vertShaderSrcStr);
}

void PolygonStyle::addData(const Json::Value& _jsonRoot, MapTile& _tile, const MapProjection& _mapProjection) {
    // Create a vboMesh which will eventually contain all the data
    std::unique_ptr<VboMesh> mesh(new VboMesh(m_vertexLayout, m_drawMode));

    //Initialize the Geometry Handler
    GeometryHandler geoHandler(_mapProjection);

    //Extract Feature Properties

    for(auto& layer : m_layerColorMap) {
        Json::Value layerFeatures = _jsonRoot[layer.first.c_str()]["features"];
        
        //iterate through all features and check for respective geometry type
        for(int i = 0; i < layerFeatures.size(); i++) {
            Json::Value geometry = layerFeatures[i]["geometry"];
            Json::Value property = layerFeatures[i]["properties"];

            std::string geomType = geometry["type"].asString();
            //if geomType not relevant type then move on
            if(geomType.compare(m_geomType) != 0) {
                continue;
            }
            //else process this geometry
            else {
                //extract feature properties
                float featureHeight  = 0.0f;
                float minFeatureHeight = 0.0f;
                if (property.isMember("height")) {
                    featureHeight = property["height"].asFloat();
                }
                if (property.isMember("min_height")) {
                    minFeatureHeight = property["min_height"].asFloat();
                }
                geoHandler.polygonAddData<vboDataUnit>(geometry["coordinates"], m_vertices, m_indices, layer.second, _tile.getOrigin(), featureHeight, minFeatureHeight);
            }
        }
    }
    //Add vertices to mesh
    mesh->addVertices((GLbyte*)&m_vertices, m_vertices.size());
    mesh->addIndices((GLushort*)&m_indices, m_indices.size());
    //1. addGeometry should take either take the name of the style or pointer to the style (this)
    //_tile.addGeometry(m_styleName, std::move(mesh));
    //_tile.addGeometry(this, std::move(mesh));
    _tile.addGeometry(*this, std::move(mesh));
}

void PolygonStyle::setup() {
}


