#include "spriteStyle.h"

#include "platform.h"
#include "tile/tile.h"
#include "gl/shaderProgram.h"
#include "gl/texture.h"
#include "gl/vertexLayout.h"
#include "util/builders.h"
#include "view/view.h"
#include "labels/spriteLabel.h"

#include "glm/gtc/type_ptr.hpp"

namespace Tangram {

SpriteStyle::SpriteStyle(std::string _name, GLenum _drawMode) : Style(_name, _drawMode) {
}

SpriteStyle::~SpriteStyle() {}

void SpriteStyle::constructVertexLayout() {

    // 32 bytes, good for memory aligments
    m_vertexLayout = std::shared_ptr<VertexLayout>(new VertexLayout({
        {"a_position", 2, GL_FLOAT, false, 0},
        {"a_uv", 2, GL_FLOAT, false, 0},
        {"a_color", 1, GL_UNSIGNED_BYTE, true, 0},
        {"a_screenPosition", 2, GL_FLOAT, false, 0},
        {"a_alpha", 1, GL_FLOAT, false, 0},
        {"a_rotation", 1, GL_FLOAT, false, 0},
    }));
}

void SpriteStyle::constructShaderProgram() {

    std::string fragShaderSrcStr = stringFromResource("sprite.fs");
    std::string vertShaderSrcStr = stringFromResource("point.vs");

    m_shaderProgram->setSourceStrings(fragShaderSrcStr, vertShaderSrcStr);

    // TODO : load this from stylesheet
    m_spriteAtlas = std::unique_ptr<SpriteAtlas>(new SpriteAtlas("poi_icons_32.png"));

    m_spriteAtlas->addSpriteNode("plane", {0, 0}, {32, 32});
    m_spriteAtlas->addSpriteNode("tree", {0, 185}, {32, 32});
    m_spriteAtlas->addSpriteNode("sunburst", {0, 629}, {32, 32});
    m_spriteAtlas->addSpriteNode("restaurant", {0, 777}, {32, 32});
    m_spriteAtlas->addSpriteNode("cafe", {0, 814}, {32, 32});
    m_spriteAtlas->addSpriteNode("museum", {0, 518}, {32, 32});
    m_spriteAtlas->addSpriteNode("bar", {0, 887}, {32, 32});
    m_spriteAtlas->addSpriteNode("train", {0, 74}, {32, 32});
    m_spriteAtlas->addSpriteNode("bus", {0, 148}, {32, 32});
    m_spriteAtlas->addSpriteNode("hospital", {0, 444}, {32, 32});
    m_spriteAtlas->addSpriteNode("parking", {0, 1073}, {32, 32});
    m_spriteAtlas->addSpriteNode("info", {0, 1110}, {32, 32});
    m_spriteAtlas->addSpriteNode("hotel", {0, 259}, {32, 32});
    m_spriteAtlas->addSpriteNode("bookstore", {0, 333}, {32, 32});
}

void SpriteStyle::buildPoint(const Point& _point, const DrawRule& _rule, const Properties& _props, VboMesh& _mesh, Tile& _tile) const {
    // TODO : make this configurable
    std::vector<Label::Vertex> vertices;

    // TODO : configure this
    float spriteScale = .5f;
    glm::vec2 offset = {0.f, 10.f};

    const static std::string key("kind");

    const std::string& kind = _props.getString(key);
    if(kind.length() == 0) { return; }

    if (!m_spriteAtlas->hasSpriteNode(kind)) {
        return;
    }

    SpriteNode spriteNode = m_spriteAtlas->getSpriteNode(kind);
    Label::Transform t = { glm::vec2(_point), glm::vec2(_point) };

    auto& mesh = static_cast<LabelMesh&>(_mesh);
    
    Label::Options options = { 0xffffff, offset };

    std::unique_ptr<SpriteLabel> label(new SpriteLabel(t, spriteNode.m_size * spriteScale, mesh, _mesh.numVertices(), options));

    auto size = spriteNode.m_size * spriteScale;
    float halfWidth = size.x * .5f;
    float halfHeight = size.y * .5f;
    const glm::vec2& uvBL = spriteNode.m_uvBL;
    const glm::vec2& uvTR = spriteNode.m_uvTR;

    vertices.reserve(4);
    vertices.push_back({{-halfWidth, -halfHeight}, {uvBL.x, uvBL.y}, options.color});
    vertices.push_back({{-halfWidth, halfHeight}, {uvBL.x, uvTR.y}, options.color});
    vertices.push_back({{halfWidth, -halfHeight}, {uvTR.x, uvBL.y}, options.color});
    vertices.push_back({{halfWidth, halfHeight}, {uvTR.x, uvTR.y}, options.color});

    mesh.addVertices(std::move(vertices), {});
    mesh.addLabel(std::move(label));
}

void SpriteStyle::onBeginDrawFrame(const View& _view, const Scene& _scene) {
    bool contextLost = Style::glContextLost();
    
    m_spriteAtlas->bind();

    static bool initUniformSampler = true;

    if (initUniformSampler || contextLost) {
        m_shaderProgram->setUniformi("u_tex", 0);
        initUniformSampler = false;
    }

    if (m_dirtyViewport || contextLost) {
        m_shaderProgram->setUniformMatrix4f("u_proj", glm::value_ptr(_view.getOrthoViewportMatrix()));
        m_dirtyViewport = false;
    }

    RenderState::blending(GL_TRUE);
    RenderState::blendingFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    RenderState::depthTest(GL_FALSE);
}

}
