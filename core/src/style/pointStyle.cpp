#include "pointStyle.h"

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

PointStyle::PointStyle(std::string _name, Blending _blendMode, GLenum _drawMode) : Style(_name, _blendMode, _drawMode) {
}

PointStyle::~PointStyle() {}

void PointStyle::constructVertexLayout() {

    m_vertexLayout = std::shared_ptr<VertexLayout>(new VertexLayout({
        {"a_position", 2, GL_FLOAT, false, 0},
        {"a_uv", 2, GL_FLOAT, false, 0},
        {"a_color", 4, GL_UNSIGNED_BYTE, true, 0},
        {"a_stroke", 4, GL_UNSIGNED_BYTE, true, 0},
        {"a_screenPosition", 2, GL_FLOAT, false, 0},
        {"a_alpha", 1, GL_FLOAT, false, 0},
        {"a_rotation", 1, GL_FLOAT, false, 0},
    }));
}

void PointStyle::constructShaderProgram() {

    std::string fragShaderSrcStr = stringFromResource("shaders/point.fs");
    std::string vertShaderSrcStr = stringFromResource("shaders/point.vs");

    m_shaderProgram->setSourceStrings(fragShaderSrcStr, vertShaderSrcStr);

    std::string defines;

    if (!m_spriteAtlas && !m_texture) {
        defines += "#define TANGRAM_POINT\n";
    }

    m_shaderProgram->addSourceBlock("defines", defines);
}

PointStyle::Parameters PointStyle::parseRule(const DrawRule& _rule) const {
    Parameters p;
    glm::vec2 size;

    // require a color or texture atlas/texture to be valid
    if (!_rule.get(StyleParamKey::color, p.color) && !m_texture && !m_spriteAtlas) {
        p.valid = false;
    }
    _rule.get(StyleParamKey::sprite, p.sprite);
    _rule.get(StyleParamKey::offset, p.offset);
    _rule.get(StyleParamKey::priority, p.priority);
    _rule.get(StyleParamKey::sprite_default, p.spriteDefault);
    if (_rule.get(StyleParamKey::size, size)) {
        if (size.x == 0.f || std::isnan(size.y)) {
            p.size = glm::vec2(size.x);
        } else {
            p.size = size;
        }
    } else {
        p.size = glm::vec2(NAN, NAN);
    }

    return p;
}

void PointStyle::buildPoint(const Point& _point, const DrawRule& _rule, const Properties& _props, VboMesh& _mesh, Tile& _tile) const {
    Parameters p = parseRule(_rule);

    if (!p.valid) {
        return;
    }

    std::vector<Label::Vertex> vertices;

    Label::Transform t = { glm::vec2(_point) };

    auto& mesh = static_cast<LabelMesh&>(_mesh);

    Label::Options options;
    options.offset = p.offset * m_pixelScale;
    options.priority = p.priority;
    options.color = p.color;

    glm::vec2 uvBL = glm::vec2(0.0);
    glm::vec2 uvTR = glm::vec2(1.0);

    if (m_spriteAtlas) {
        SpriteNode spriteNode;

        if (!m_spriteAtlas->getSpriteNode(p.sprite, spriteNode) && !m_spriteAtlas->getSpriteNode(p.spriteDefault, spriteNode)) {
            return;
        }

        if (std::isnan(p.size.x)) {
            p.size = spriteNode.m_size;
        }

        uvBL = spriteNode.m_uvBL;
        uvTR = spriteNode.m_uvTR;
    } else {
        // default point size
        if (std::isnan(p.size.x)) {
            p.size = glm::vec2(8.0);
        }
    }

    std::unique_ptr<SpriteLabel> label(new SpriteLabel(t, p.size * m_pixelScale, mesh, _mesh.numVertices(), options));

    float halfWidth = p.size.x * .5f;
    float halfHeight = p.size.y * .5f;

    vertices.reserve(4);
    vertices.push_back({{-halfWidth, -halfHeight}, {uvBL.x, uvBL.y}, options.color});
    vertices.push_back({{-halfWidth, halfHeight}, {uvBL.x, uvTR.y}, options.color});
    vertices.push_back({{halfWidth, -halfHeight}, {uvTR.x, uvBL.y}, options.color});
    vertices.push_back({{halfWidth, halfHeight}, {uvTR.x, uvTR.y}, options.color});

    mesh.addVertices(std::move(vertices), {});
    mesh.addLabel(std::move(label));
}


void PointStyle::buildLine(const Line& _line, const DrawRule& _rule, const Properties& _props, VboMesh& _mesh, Tile& _tile) const {
    for (size_t i = 0; i < _line.size(); ++i) {
        Point p = glm::vec3(glm::vec2(_line[i]), 0.0);
        buildPoint(p, _rule, _props, _mesh, _tile);
    }
}

void PointStyle::buildPolygon(const Polygon& _polygon, const DrawRule& _rule, const Properties& _props, VboMesh& _mesh, Tile& _tile) const {
    Point p = glm::vec3(centroid(_polygon), 0.0);
    buildPoint(p, _rule, _props, _mesh, _tile);
}

void PointStyle::onBeginDrawFrame(const View& _view, Scene& _scene) {
    bool contextLost = Style::glContextLost();

    if (m_spriteAtlas) {
        m_spriteAtlas->bind(0);
    } else if (m_texture) {
        m_texture->update(0);
        m_texture->bind(0);
    }

    setupShaderUniforms(1, contextLost, _scene);

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
