#include "debugStyle.h"

#include "tangram.h"

#include <vector>
#include <memory>
#include <string>

DebugStyle::DebugStyle(std::string _name, GLenum _drawMode) : Style(_name, _drawMode) {
    
    constructVertexLayout();
    constructShaderProgram();
    
}

void DebugStyle::constructVertexLayout() {
    
    m_vertexLayout = std::shared_ptr<VertexLayout>(new VertexLayout({
        {"a_position", 3, GL_FLOAT, false, 0},
        {"a_color", 4, GL_UNSIGNED_BYTE, true, 0}
    }));
    
}

void DebugStyle::constructShaderProgram() {
    
    std::string vertShaderSrcStr = stringFromResource("debug.vs");
    std::string fragShaderSrcStr = stringFromResource("debug.fs");
    
    m_shaderProgram = std::make_shared<ShaderProgram>();
    m_shaderProgram->setSourceStrings(fragShaderSrcStr, vertShaderSrcStr);
    
}

void DebugStyle::addData(TileData &_data, MapTile &_tile, const MapProjection &_mapProjection) const {
    
    if (Tangram::getDebugFlag(Tangram::DebugFlags::TILE_BOUNDS)) {
        
        Mesh* mesh = new Mesh(m_vertexLayout, m_drawMode);
        
        // Add four vertices to draw the outline of the tile in red
        
        std::vector<PosColVertex> vertices;
        
        GLuint abgr = 0xff0000ff;
        
        vertices.reserve(4);
        vertices.push_back({ -1.f, -1.f, 0.f, abgr });
        vertices.push_back({  1.f, -1.f, 0.f, abgr });
        vertices.push_back({  1.f,  1.f, 0.f, abgr });
        vertices.push_back({ -1.f,  1.f, 0.f, abgr });
        
        mesh->addVertices(std::move(vertices), { 0, 1, 2, 3, 0 });
        mesh->compileVertexBuffer();
        
        _tile.addGeometry(*this, std::unique_ptr<VboMesh>(mesh));
        
    }
    
}

void DebugStyle::buildPoint(Point &_point, std::string &_layer, Properties &_props, VboMesh &_mesh) const {
    
    // No-op
    
}

void DebugStyle::buildLine(Line &_line, std::string &_layer, Properties &_props, VboMesh &_mesh) const {
    
    // No-op
    
}

void DebugStyle::buildPolygon(Polygon &_polygon, std::string &_layer, Properties &_props, VboMesh &_mesh) const {
    
    // No-op
    
}
