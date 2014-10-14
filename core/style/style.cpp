#include "style.h"
#include "platform.h"
#include "util/geometryHandler.h"
#include "util/jsonExtractor.h"

/*
 * Style Class Methods
 */

Style::Style(std::string _geomType, std::string _styleName, GLenum _drawMode) : m_geomType(_geomType), m_styleName(_styleName), m_drawMode(_drawMode) {
}

void Style::setFragShaderSrc(std::string _fragShaderSrcStr) {
    m_fragShaderSrcStr = std::move(_fragShaderSrcStr);
}

void Style::setVertShaderSrc(std::string _vertShaderSrcStr) {
    m_vertShaderSrcStr = std::move(_vertShaderSrcStr);
}

void Style::updateLayers(std::vector<std::pair<std::string, GLuint>> _newLayers) {
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

PolygonStyle::PolygonStyle(std::string _geomType, std::string _styleName, GLenum _drawMode) : Style(_geomType, _styleName, _drawMode) {

    // Pass in a fileName or a url to construct shader strings from
    m_vertShaderSrcStr =
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

PolygonStyle::PolygonStyle(std::string _geomType, GLenum _drawMode) : PolygonStyle(_geomType, _geomType, _drawMode) {}

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

    //Extract Feature Properties
    for(auto& layer : m_layerColorMap) {
        Json::Value layerFeatures = _jsonRoot[layer.first.c_str()]["features"];
        //iterate through all features and check for respective geometry type
        for(int i = 0; i < layerFeatures.size(); i++) {
            if(m_geomType.compare(JsonExtractor::extractGeomType(layerFeatures[i])) == 0) {
                std::vector<glm::vec3> extractedGeomCoords;
                std::vector<int> polyRingSizes;
                JsonExtractor::extractGeomCoords(extractedGeomCoords, polyRingSizes, layerFeatures[i], _tile.getOrigin(), _mapProjection);
                GeometryHandler::buildPolygon(extractedGeomCoords, polyRingSizes, m_vertCoords, m_vertNormals, m_indices);
                //GeometryHandler::buildPolygonExtrusion(JsonExtractor::extractGeomCoords(layerFeatures, _tile.getOrigin(), _mapProjection), m_vertCoords, m_vertNormals, m_indices);
                /* fill style's m_vertices with vertCoord, vertNormal and color data */
                fillVertexData(layer.second);
            }
            else {
                continue;
            }
        }
    }
    if(m_vertices.size() == 0) {
        //Nothing to add to mesh, get rid of mesh
        mesh.reset(nullptr);
    }
    else {
        //Add vertices to mesh
        mesh->addVertices((GLbyte*)m_vertices.data(), m_vertices.size());
        mesh->addIndices((GLushort*)m_indices.data(), m_indices.size());
        //1. addGeometry should take either take the name of the style or pointer to the style (this)
        _tile.addGeometry(*this, std::move(mesh));
    }
}

void PolygonStyle::fillVertexData(const GLuint& _abgr) {
    for(int i = m_vertices.size(); i < m_vertCoords.size(); i++) {
        m_vertices.push_back( { m_vertCoords[i].x, m_vertCoords[i].y, m_vertCoords[i].z,
                                m_vertNormals[i].x, m_vertNormals[i].y, m_vertNormals[i].z,
                                _abgr
                              });
    }
    return;
}

void PolygonStyle::clearStyleData() {
    m_vertices.clear();
    m_indices.clear();
    m_vertCoords.clear();
    m_vertNormals.clear();
}

void PolygonStyle::setup() {
}


/*
 * MultiPolygon Style Class Methods
 */

MultiPolygonStyle::MultiPolygonStyle(std::string _geomType, std::string _styleName, GLenum _drawMode) : Style(_geomType, _styleName, _drawMode) {

    // Pass in a fileName or a url to construct shader strings from
    m_vertShaderSrcStr =
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

MultiPolygonStyle::MultiPolygonStyle(std::string _geomType, GLenum _drawMode) : MultiPolygonStyle(_geomType, _geomType, _drawMode) {}

void MultiPolygonStyle::constructVertexLayout() {
    // fixed per style... this is basic position, normal color
    m_vertexLayout = std::shared_ptr<VertexLayout>(new VertexLayout({
        {"a_position", 3, GL_FLOAT, false, 0},
        {"a_normal", 3, GL_FLOAT, false, 0},
        {"a_color", 4, GL_UNSIGNED_BYTE, true, 0}
    }));
}

void MultiPolygonStyle::constructShaderProgram() {
    m_shaderProgram = std::shared_ptr<ShaderProgram>(new ShaderProgram());
    m_shaderProgram->buildFromSourceStrings(m_fragShaderSrcStr, m_vertShaderSrcStr);
}

void MultiPolygonStyle::addData(const Json::Value& _jsonRoot, MapTile& _tile, const MapProjection& _mapProjection) {
    // Create a vboMesh which will eventually contain all the data
    std::unique_ptr<VboMesh> mesh(new VboMesh(m_vertexLayout, m_drawMode));

    //Extract Feature Properties
    for(auto& layer : m_layerColorMap) {
        Json::Value layerFeatures = _jsonRoot[layer.first.c_str()]["features"];
        //iterate through all features and check for respective geometry type
        for(int i = 0; i < layerFeatures.size(); i++) {
            if(m_geomType.compare(JsonExtractor::extractGeomType(layerFeatures[i])) == 0) {
                int numPolys = JsonExtractor::extractNumPoly(layerFeatures[i]);
                std::vector<glm::vec3> extractedGeomCoords;
                std::vector<int> polyRingSizes;
                JsonExtractor::extractGeomCoords(extractedGeomCoords, polyRingSizes, layerFeatures[i], _tile.getOrigin(), _mapProjection, numPolys);
                GeometryHandler::buildPolygon(extractedGeomCoords, polyRingSizes, m_vertCoords, m_vertNormals, m_indices);
                //GeometryHandler::buildPolygonExtrusion(JsonExtractor::extractGeomCoords(layerFeatures, _tile.getOrigin(), _mapProjection), m_vertCoords, m_vertNormals, m_indices);
                /* fill style's m_vertices with vertCoord, vertNormal and color data */
                fillVertexData(layer.second);
            }
            else {
                continue;
            }
        }
    }
    if(m_vertices.size() == 0) {
        //Nothing to add to mesh, get rid of mesh
        mesh.reset(nullptr);
    }
    else {
        //Add vertices to mesh
        mesh->addVertices((GLbyte*)m_vertices.data(), m_vertices.size());
        mesh->addIndices((GLushort*)m_indices.data(), m_indices.size());
        //1. addGeometry should take either take the name of the style or pointer to the style (this)
        _tile.addGeometry(*this, std::move(mesh));
    }
}

void MultiPolygonStyle::fillVertexData(const GLuint& _abgr) {
    for(int i = m_vertices.size(); i < m_vertCoords.size(); i++) {
        m_vertices.push_back( { m_vertCoords[i].x, m_vertCoords[i].y, m_vertCoords[i].z,
                                m_vertNormals[i].x, m_vertNormals[i].y, m_vertNormals[i].z,
                                _abgr
                              });
    }
    return;
}

void MultiPolygonStyle::clearStyleData() {
    m_vertices.clear();
    m_indices.clear();
    m_vertCoords.clear();
    m_vertNormals.clear();
}

void MultiPolygonStyle::setup() {
}
