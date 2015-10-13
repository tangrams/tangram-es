#include "pointStyle.h"

#include "platform.h"
#include "tile/tile.h"
#include "gl/shaderProgram.h"
#include "gl/texture.h"
#include "gl/vertexLayout.h"
#include "util/builders.h"
#include "view/view.h"
#include "labels/spriteLabel.h"
#include "data/propertyItem.h" // Include wherever Properties is used!

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

    std::string fragShaderSrcStr = stringFromFile("shaders/point.fs", PathType::internal);
    std::string vertShaderSrcStr = stringFromFile("shaders/point.vs", PathType::internal);

    m_shaderProgram->setSourceStrings(fragShaderSrcStr, vertShaderSrcStr);

    std::string defines;

    if (!m_spriteAtlas && !m_texture) {
        defines += "#define TANGRAM_POINT\n";
    }

    m_shaderProgram->addSourceBlock("defines", defines);
}

PointStyle::Parameters PointStyle::applyRule(const DrawRule& _rule) const {

    Parameters p;
    glm::vec2 size;

    // require a color or texture atlas/texture to be valid
    if (!_rule.get(StyleParamKey::color, p.color) && !m_texture && !m_spriteAtlas) {
        p.valid = false;
        return p;
    }
    _rule.get(StyleParamKey::sprite, p.sprite);
    _rule.get(StyleParamKey::offset, p.offset);
    _rule.get(StyleParamKey::priority, p.priority);
    _rule.get(StyleParamKey::sprite_default, p.spriteDefault);
    _rule.get(StyleParamKey::centroid, p.centroid);
    _rule.get(StyleParamKey::interactive, p.interactive);

    auto sizeParam = _rule.findParameter(StyleParamKey::size);
    if (sizeParam.stops && sizeParam.value.is<float>()) {
        p.size = glm::vec2(sizeParam.value.get<float>());
    } else if (_rule.get(StyleParamKey::size, size)) {
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

void PointStyle::pushQuad(std::vector<Label::Vertex>& _vertices, const glm::vec2& _size,
                          const glm::vec2& _uvBL, const glm::vec2& _uvTR, unsigned int _color) const {
    _vertices.push_back({{-_size.x, -_size.y}, {_uvBL.x, _uvBL.y}, _color});
    _vertices.push_back({{-_size.x, _size.y}, {_uvBL.x, _uvTR.y}, _color});
    _vertices.push_back({{_size.x, -_size.y}, {_uvTR.x, _uvBL.y}, _color});
    _vertices.push_back({{_size.x, _size.y}, {_uvTR.x, _uvTR.y}, _color});
}

Label::Options PointStyle::optionsFromPointParams(const Parameters& _params) const {
    Label::Options options;
    options.offset = _params.offset * m_pixelScale;
    options.priority = _params.priority;
    options.color = _params.color;
    options.interactive = _params.interactive;

    return options;
}

bool PointStyle::getUVQuad(Parameters& _params, glm::vec4& _quad) const {
    _quad = glm::vec4(0.0, 0.0, 1.0, 1.0);

    if (m_spriteAtlas) {
        SpriteNode spriteNode;

        if (!m_spriteAtlas->getSpriteNode(_params.sprite, spriteNode) &&
            !m_spriteAtlas->getSpriteNode(_params.spriteDefault, spriteNode)) {
            return false;
        }

        if (std::isnan(_params.size.x)) {
            _params.size = spriteNode.m_size;
        }

        _quad.x = spriteNode.m_uvBL.x;
        _quad.y = spriteNode.m_uvBL.y;
        _quad.z = spriteNode.m_uvTR.x;
        _quad.w = spriteNode.m_uvTR.y;
    } else {
        // default point size
        if (std::isnan(_params.size.x)) {
            _params.size = glm::vec2(8.0);
        }
    }

    _params.size *= m_pixelScale;

    return true;
}

void PointStyle::buildPoint(const Point& _point, const DrawRule& _rule, const Properties& _props,
                            VboMesh& _mesh, Tile& _tile) const {
    Parameters p = applyRule(_rule);
    glm::vec4 uvsQuad;

    if (!p.valid || !getUVQuad(p, uvsQuad)) {
        return;
    }

    auto& mesh = static_cast<LabelMesh&>(_mesh);
    Label::Transform transform = { glm::vec2(_point) };
    Label::Options options = optionsFromPointParams(p);
    if (p.interactive) {
        options.properties = std::make_shared<Properties>(_props);
    }

    mesh.addLabel(std::make_unique<SpriteLabel>(transform, p.size, mesh, _mesh.numVertices(), options));

    std::vector<Label::Vertex> vertices;

    vertices.reserve(4);
    pushQuad(vertices, p.size * 0.5f, {uvsQuad.x, uvsQuad.y}, {uvsQuad.z, uvsQuad.w}, options.color);
    mesh.addVertices(std::move(vertices), {});
}

void PointStyle::buildLine(const Line& _line, const DrawRule& _rule, const Properties& _props,
                           VboMesh& _mesh, Tile& _tile) const {
    Parameters p = applyRule(_rule);
    glm::vec4 uvsQuad;

    if (!p.valid || !getUVQuad(p, uvsQuad)) {
        return;
    }

    std::vector<Label::Vertex> vertices;
    auto& mesh = static_cast<LabelMesh&>(_mesh);

    vertices.reserve(4 * _line.size());
    Label::Options options = optionsFromPointParams(p);
    if (p.interactive) {
        options.properties = std::make_shared<Properties>(_props);
    }

    for (size_t i = 0; i < _line.size(); ++i) {
        Label::Transform transform = { glm::vec2(_line[i]) };

        mesh.addLabel(std::make_unique<SpriteLabel>(transform, p.size, mesh, _mesh.numVertices(), options));
        pushQuad(vertices, p.size * 0.5f, {uvsQuad.x, uvsQuad.y}, {uvsQuad.z, uvsQuad.w}, options.color);
    }

    mesh.addVertices(std::move(vertices), {});
}

void PointStyle::buildPolygon(const Polygon& _polygon, const DrawRule& _rule, const Properties& _props,
                              VboMesh& _mesh, Tile& _tile) const {
    Parameters p = applyRule(_rule);
    glm::vec4 uvsQuad;

    if (!p.valid || !getUVQuad(p, uvsQuad)) {
        return;
    }

    Label::Options options = optionsFromPointParams(p);
    if (p.interactive) {
        options.properties = std::make_shared<Properties>(_props);
    }

    std::vector<Label::Vertex> vertices;
    auto& mesh = static_cast<LabelMesh&>(_mesh);

    if (!p.centroid) {

        int size = 0;
        for (auto line : _polygon) { size += line.size(); }

        vertices.reserve(size);

        for (auto line : _polygon) {
            for (auto point : line) {
                Label::Transform transform = { glm::vec2(point) };

                mesh.addLabel(std::make_unique<SpriteLabel>(transform, p.size, mesh, _mesh.numVertices(), options));
                pushQuad(vertices, p.size * 0.5f, {uvsQuad.x, uvsQuad.y}, {uvsQuad.z, uvsQuad.w}, options.color);
            }
        }
    } else {
        vertices.reserve(4);
        glm::vec2 c = centroid(_polygon);
        Label::Transform transform = { c };

        mesh.addLabel(std::make_unique<SpriteLabel>(transform, p.size, mesh, _mesh.numVertices(), options));
        pushQuad(vertices, p.size * 0.5f, {uvsQuad.x, uvsQuad.y}, {uvsQuad.z, uvsQuad.w}, options.color);
    }

    mesh.addVertices(std::move(vertices), {});
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
        m_shaderProgram->setUniformMatrix4f("u_ortho", glm::value_ptr(_view.getOrthoViewportMatrix()));
        m_dirtyViewport = false;
    }

    Style::onBeginDrawFrame(_view, _scene);
}

}
