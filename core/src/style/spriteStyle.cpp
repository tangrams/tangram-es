#include "spriteStyle.h"

#include "platform.h"
#include "tile/mapTile.h"
#include "util/shaderProgram.h"
#include "util/texture.h"
#include "util/vertexLayout.h"

SpriteStyle::SpriteStyle(std::string _name, GLenum _drawMode) : Style(_name, _drawMode) {}

SpriteStyle::~SpriteStyle() {}

void SpriteStyle::constructVertexLayout() {

    m_vertexLayout = std::shared_ptr<VertexLayout>(new VertexLayout({
        {"a_position", 3, GL_FLOAT, false, 0}, {"a_uv", 2, GL_FLOAT, false, 0},
    }));
}

void SpriteStyle::constructShaderProgram() {

    std::string fragShaderSrcStr = stringFromResource("texture.fs");
    std::string vertShaderSrcStr = stringFromResource("texture.vs");

    m_shaderProgram->setSourceStrings(fragShaderSrcStr, vertShaderSrcStr);

    m_texture = std::shared_ptr<Texture>(new Texture("mapzen-logo.png"));
}

void SpriteStyle::buildPoint(Point& _point, const StyleParamMap& _styleParamMap, Properties& _props, VboMesh& _mesh) const {}

void SpriteStyle::buildLine(Line& _line, const StyleParamMap& _styleParamMap, Properties& _props, VboMesh& _mesh) const {}

void SpriteStyle::buildPolygon(Polygon& _polygon, const StyleParamMap& _styleParamMap, Properties& _props, VboMesh& _mesh) const {}

void SpriteStyle::onBeginDrawFrame(const std::shared_ptr<View>& _view, const std::shared_ptr<Scene>& _scene) {
    m_texture->update(0);
    m_texture->bind(0);
    m_shaderProgram->setUniformi("u_tex", 0);
}

void SpriteStyle::addData(TileData& _data, MapTile& _tile) {

    Mesh* mesh = new Mesh(m_vertexLayout, m_drawMode);

    std::vector<PosUVVertex> vertices;

    vertices.reserve(4);

    float size = 0.2;
    vertices.push_back({{size, size, 0.f}, {1.f, 0.f}});
    vertices.push_back({{-size, size, 0.f}, {0.f, 0.f}});
    vertices.push_back({{-size, -size, 0.f}, {0.f, 1.f}});
    vertices.push_back({{size, -size, 0.f}, {1.f, 1.f}});

    mesh->addVertices(std::move(vertices), {0, 1, 2, 2, 3, 0});
    mesh->compileVertexBuffer();

    _tile.addGeometry(*this, std::unique_ptr<VboMesh>(mesh));
}
