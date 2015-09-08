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

SpriteStyle::SpriteStyle(std::string _name, Blending _blendMode, GLenum _drawMode) : Style(_name, _blendMode, _drawMode) {
}

SpriteStyle::~SpriteStyle() {}

void SpriteStyle::constructVertexLayout() {

    // 32 bytes, good for memory aligments
    m_vertexLayout = std::shared_ptr<VertexLayout>(new VertexLayout({
        {"a_position", 2, GL_FLOAT, false, 0},
        {"a_uv", 2, GL_FLOAT, false, 0},
        {"a_color", 4, GL_UNSIGNED_BYTE, true, 0},
        {"a_screenPosition", 2, GL_FLOAT, false, 0},
        {"a_alpha", 1, GL_FLOAT, false, 0},
        {"a_rotation", 1, GL_FLOAT, false, 0},
    }));
}

void SpriteStyle::constructShaderProgram() {

    std::string fragShaderSrcStr = stringFromResource("sprite.fs");
    std::string vertShaderSrcStr = stringFromResource("point.vs");

    m_shaderProgram->setSourceStrings(fragShaderSrcStr, vertShaderSrcStr);
}

void SpriteStyle::buildPoint(const Point& _point, const DrawRule& _rule, const Properties& _props, VboMesh& _mesh, Tile& _tile) const {
    // TODO : make this configurable
    std::vector<Label::Vertex> vertices;

    // TODO : configure this
    float spriteScale = .5f;
    glm::vec2 offset = {0.f, 10.f};

    const static std::string key("kind");

    std::string spriteName;
    if (!_rule.get(StyleParamKey::sprite, spriteName)) {
        spriteName = _props.getString(key);
    }
    if (spriteName.empty() || !m_spriteAtlas->hasSpriteNode(spriteName)) {
        return;
    }

    SpriteNode spriteNode = m_spriteAtlas->getSpriteNode(spriteName);
    Label::Transform t = { glm::vec2(_point) };

    auto& mesh = static_cast<LabelMesh&>(_mesh);

    Label::Options options;
    options.offset = offset;

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

    if (!m_spriteAtlas) {
        return;
    }

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

    Style::onBeginDrawFrame(_view, _scene);
}

}
