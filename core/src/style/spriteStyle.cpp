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

    SpriteBuilder builder = {
        [&](const glm::vec2& coord, const glm::vec2& screenPos, const glm::vec2& uv) {
            vertices.push_back({ coord, uv, { screenPos, 0.5f, M_PI_2 } });
        }
    };

    // TODO : configure this
    float spriteScale = .5f;
    glm::vec2 offset = {0.f, 10.f};

    auto it = _props.stringProps.find("kind");
    if (it == _props.stringProps.end()) {
        return;
    }

    const std::string& kind = (*it).second;
    if (!m_spriteAtlas->hasSpriteNode(kind)) {
        return;
    }

    SpriteNode spriteNode = m_spriteAtlas->getSpriteNode(kind);
    Label::Transform t = { glm::vec2(_point), glm::vec2(_point), offset };

    size_t bufferOffset = _mesh.numVertices() * m_vertexLayout->getStride();

    auto& mesh = static_cast<LabelMesh&>(_mesh);

    std::unique_ptr<SpriteLabel> label(new SpriteLabel(mesh, t, spriteNode.m_size * spriteScale, bufferOffset));

    Builders::buildQuadAtPoint(label->getTransform().state.screenPos + offset,
                               spriteNode.m_size * spriteScale,
                               spriteNode.m_uvBL, spriteNode.m_uvTR, builder);

    mesh.addVertices(std::move(vertices), std::move(builder.indices));
    mesh.addLabel(std::move(label));
}

void SpriteStyle::onBeginDrawFrame(const View& _view, const Scene& _scene) {
    m_spriteAtlas->bind();

    static bool initUniformSampler = true;

    if (initUniformSampler) {
        m_shaderProgram->setUniformi("u_tex", 0);
        initUniformSampler = false;
    }

    if (true || m_dirtyViewport) {
        m_shaderProgram->setUniformMatrix4f("u_proj", glm::value_ptr(_view.getOrthoViewportMatrix()));
        m_dirtyViewport = false;
    }

    RenderState::blending(GL_TRUE);
    RenderState::blendingFunc({GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA});
    RenderState::depthTest(GL_FALSE);
}

}
