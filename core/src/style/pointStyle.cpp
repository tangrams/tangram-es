#include "pointStyle.h"

#include "platform.h"
#include "material.h"
#include "gl/shaderProgram.h"
#include "gl/texture.h"
#include "gl/vertexLayout.h"
#include "labels/labelMesh.h"
#include "labels/spriteLabel.h"
#include "scene/drawRule.h"
#include "scene/spriteAtlas.h"
#include "tile/tile.h"
#include "util/builders.h"
#include "view/view.h"
#include "data/propertyItem.h" // Include wherever Properties is used!
#include "scene/stops.h"

namespace Tangram {

PointStyle::PointStyle(std::string _name, Blending _blendMode, GLenum _drawMode) : Style(_name, _blendMode, _drawMode) {
}

PointStyle::~PointStyle() {}

void PointStyle::constructVertexLayout() {

    m_vertexLayout = std::shared_ptr<VertexLayout>(new VertexLayout({
        {"a_position", 2, GL_FLOAT, false, 0},
        {"a_uv", 2, GL_FLOAT, false, 0},
        {"a_extrude", 3, GL_FLOAT, false, 0},
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

VboMesh* PointStyle::newMesh() const {
    return new LabelMesh(m_vertexLayout, m_drawMode);
}

bool PointStyle::checkRule(const DrawRule& _rule) const {
    uint32_t checkColor;
    // require a color or texture atlas/texture to be valid
    if (!_rule.get(StyleParamKey::color, checkColor) && !m_texture && !m_spriteAtlas) {
        return false;
    }
    return true;
}

PointStyle::Parameters PointStyle::applyRule(const DrawRule& _rule, const Properties& _props, float _zoom) const {

    Parameters p;
    glm::vec2 size;
    std::string anchor;

    _rule.get(StyleParamKey::color, p.color);
    _rule.get(StyleParamKey::sprite, p.sprite);
    _rule.get(StyleParamKey::offset, p.labelOptions.offset);
    _rule.get(StyleParamKey::priority, p.labelOptions.priority);
    _rule.get(StyleParamKey::sprite_default, p.spriteDefault);
    _rule.get(StyleParamKey::centroid, p.centroid);
    _rule.get(StyleParamKey::interactive, p.labelOptions.interactive);
    _rule.get(StyleParamKey::collide, p.labelOptions.collide);
    _rule.get(StyleParamKey::transition_hide_time, p.labelOptions.hideTransition.time);
    _rule.get(StyleParamKey::transition_selected_time, p.labelOptions.selectTransition.time);
    _rule.get(StyleParamKey::transition_show_time, p.labelOptions.showTransition.time);
    _rule.get(StyleParamKey::anchor, anchor);

    auto sizeParam = _rule.findParameter(StyleParamKey::size);
    if (sizeParam.stops && sizeParam.value.is<float>()) {
        float lowerSize = sizeParam.value.get<float>();
        float higherSize = sizeParam.stops->evalWidth(_zoom + 1);
        p.extrudeScale = (higherSize - lowerSize) * 0.5f - 1.f;
        p.size = glm::vec2(lowerSize);
    } else if (_rule.get(StyleParamKey::size, size)) {
        if (size.x == 0.f || std::isnan(size.y)) {
            p.size = glm::vec2(size.x);
        } else {
            p.size = size;
        }
    } else {
        p.size = glm::vec2(NAN, NAN);
    }

    LabelProperty::anchor(anchor, p.anchor);

    if (p.labelOptions.interactive) {
        p.labelOptions.properties = std::make_shared<Properties>(_props);
    }

    return p;
}

void PointStyle::pushQuad(std::vector<Label::Vertex>& _vertices, const glm::vec2& _size, const glm::vec2& _uvBL,
                          const glm::vec2& _uvTR, unsigned int _color, float _extrudeScale) const {

    _vertices.push_back({{    0.0,       0.0}, {_uvBL.x, _uvTR.y}, {-1.f,  1.f, _extrudeScale}, _color});
    _vertices.push_back({{_size.x,       0.0}, {_uvTR.x, _uvTR.y}, { 1.f,  1.f, _extrudeScale}, _color});
    _vertices.push_back({{    0.0,  -_size.y}, {_uvBL.x, _uvBL.y}, {-1.f, -1.f, _extrudeScale}, _color});
    _vertices.push_back({{_size.x,  -_size.y}, {_uvTR.x, _uvBL.y}, { 1.f, -1.f, _extrudeScale}, _color});
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
    Parameters p = applyRule(_rule, _props, _tile.getID().z);
    glm::vec4 uvsQuad;

    if (!getUVQuad(p, uvsQuad)) {
        return;
    }

    auto& mesh = static_cast<LabelMesh&>(_mesh);
    Label::Transform transform = { glm::vec2(_point) };

    mesh.addLabel(std::make_unique<SpriteLabel>(transform, p.size, mesh, _mesh.numVertices(),
                p.labelOptions, p.extrudeScale, p.anchor));

    std::vector<Label::Vertex> vertices;

    vertices.reserve(4);
    pushQuad(vertices, p.size, {uvsQuad.x, uvsQuad.y}, {uvsQuad.z, uvsQuad.w},
            p.color, p.extrudeScale);
    mesh.addVertices(std::move(vertices), {});
}

void PointStyle::buildLine(const Line& _line, const DrawRule& _rule, const Properties& _props,
                           VboMesh& _mesh, Tile& _tile) const {
    Parameters p = applyRule(_rule, _props, _tile.getID().z);
    glm::vec4 uvsQuad;

    if (!getUVQuad(p, uvsQuad)) {
        return;
    }

    std::vector<Label::Vertex> vertices;
    auto& mesh = static_cast<LabelMesh&>(_mesh);

    vertices.reserve(4 * _line.size());

    for (size_t i = 0; i < _line.size(); ++i) {
        Label::Transform transform = { glm::vec2(_line[i]) };

        mesh.addLabel(std::make_unique<SpriteLabel>(transform, p.size, mesh, _mesh.numVertices(),
                    p.labelOptions, p.extrudeScale, p.anchor));
        pushQuad(vertices, p.size, {uvsQuad.x, uvsQuad.y}, {uvsQuad.z, uvsQuad.w},
                p.color, p.extrudeScale);
    }

    mesh.addVertices(std::move(vertices), {});
}

void PointStyle::buildPolygon(const Polygon& _polygon, const DrawRule& _rule, const Properties& _props,
                              VboMesh& _mesh, Tile& _tile) const {
    Parameters p = applyRule(_rule, _props, _tile.getID().z);
    glm::vec4 uvsQuad;

    if (!getUVQuad(p, uvsQuad)) {
        return;
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

                mesh.addLabel(std::make_unique<SpriteLabel>(transform, p.size, mesh, _mesh.numVertices(),
                            p.labelOptions, p.extrudeScale, p.anchor));
                pushQuad(vertices, p.size, {uvsQuad.x, uvsQuad.y}, {uvsQuad.z, uvsQuad.w},
                        p.color, p.extrudeScale);
            }
        }
    } else {
        vertices.reserve(4);
        glm::vec2 c = centroid(_polygon);
        Label::Transform transform = { c };

        mesh.addLabel(std::make_unique<SpriteLabel>(transform, p.size, mesh, _mesh.numVertices(),
                    p.labelOptions, p.extrudeScale, p.anchor));
        pushQuad(vertices, p.size,
                {uvsQuad.x, uvsQuad.y}, {uvsQuad.z, uvsQuad.w}, p.color, p.extrudeScale);
    }

    mesh.addVertices(std::move(vertices), {});
}

void PointStyle::onBeginDrawFrame(const View& _view, Scene& _scene, int _textureUnit) {
    if (m_spriteAtlas) {
        m_spriteAtlas->bind(0);
    } else if (m_texture) {
        m_texture->update(0);
        m_texture->bind(0);
    }

    m_shaderProgram->setUniformi("u_tex", 0);
    m_shaderProgram->setUniformMatrix4f("u_ortho", _view.getOrthoViewportMatrix());

    Style::onBeginDrawFrame(_view, _scene, 1);
}

}
