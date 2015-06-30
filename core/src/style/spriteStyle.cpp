#include "spriteStyle.h"
#include "texture.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

SpriteStyle::SpriteStyle(std::string _name, GLenum _drawMode) : Style(_name, _drawMode) {
}

SpriteStyle::~SpriteStyle() {}

void SpriteStyle::constructVertexLayout() {

    m_vertexLayout = std::shared_ptr<VertexLayout>(new VertexLayout({
        {"a_position", 2, GL_FLOAT, false, 0},
        {"a_uv", 2, GL_FLOAT, false, 0},
    }));

}

void SpriteStyle::constructShaderProgram() {

    std::string fragShaderSrcStr = stringFromResource("sprite.fs");
    std::string vertShaderSrcStr = stringFromResource("sprite.vs");

    m_shaderProgram->setSourceStrings(fragShaderSrcStr, vertShaderSrcStr);

    m_spriteAtlas = std::unique_ptr<SpriteAtlas>(new SpriteAtlas("poi_icons_32.png"));
    
    m_spriteAtlas->addSpriteNode("plane", {0, 0}, {32, 32});
    m_spriteAtlas->addSpriteNode("tree", {0, 185}, {32, 32});
    m_spriteAtlas->addSpriteNode("sunburst", {0, 629}, {32, 32});
}

void* SpriteStyle::parseStyleParams(const std::string& _layerNameID, const StyleParamMap& _styleParamMap) {
    return nullptr;
}

void SpriteStyle::buildPoint(Point& _point, void* _styleParam, Properties& _props, VboMesh& _mesh) const {

}

void SpriteStyle::buildLine(Line& _line, void* _styleParam, Properties& _props, VboMesh& _mesh) const {

}

void SpriteStyle::buildPolygon(Polygon& _polygon, void* _styleParam, Properties& _props, VboMesh& _mesh) const {

}

void SpriteStyle::onBeginDrawFrame(const std::shared_ptr<View>& _view, const std::shared_ptr<Scene>& _scene) {

    m_spriteAtlas->bind();
    
    m_shaderProgram->setUniformi("u_tex", 0);
    
    // top-left screen axis, y pointing down
    m_shaderProgram->setUniformMatrix4f("u_proj", glm::value_ptr(glm::ortho(0.f, _view->getWidth(), _view->getHeight(), 0.f, -1.f, 1.f)));
    
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void SpriteStyle::onEndDrawFrame() {
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}

void SpriteStyle::addData(TileData& _data, MapTile& _tile, const MapProjection& _mapProjection) {

    Mesh* mesh = new Mesh(m_vertexLayout, m_drawMode);

    std::vector<PosUVVertex> vertices;

    SpriteBuilder builder = {
        [&](const glm::vec2& coord, const glm::vec2& uv) {
            vertices.push_back({ coord, uv });
        }
    };
    
    SpriteNode planeSprite = m_spriteAtlas->getSpriteNode("plane");
    SpriteNode treeSprite = m_spriteAtlas->getSpriteNode("tree");
    SpriteNode sunburstSprite = m_spriteAtlas->getSpriteNode("sunburst");
    
    // Test building different sprite quads
    Builders::buildQuadAtPoint({150, 150}, planeSprite.size, planeSprite.uvBL, planeSprite.uvTR, builder);
    Builders::buildQuadAtPoint({350, 150}, treeSprite.size, treeSprite.uvBL, treeSprite.uvTR, builder);
    Builders::buildQuadAtPoint({150, 350}, sunburstSprite.size, sunburstSprite.uvBL, sunburstSprite.uvTR, builder);
    
    mesh->addVertices(std::move(vertices), std::move(builder.indices));
    mesh->compileVertexBuffer();

    _tile.addGeometry(*this, std::shared_ptr<VboMesh>(mesh));

}
