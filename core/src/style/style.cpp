#include "style.h"

#include "material.h"
#include "gl/renderState.h"
#include "gl/shaderProgram.h"
#include "gl/vboMesh.h"
#include "scene/light.h"
#include "scene/styleParam.h"
#include "scene/drawRule.h"
#include "scene/scene.h"
#include "scene/spriteAtlas.h"
#include "tile/tile.h"
#include "view/view.h"

namespace Tangram {

    Style::Style(std::string _name, Blending _blendMode, GLenum _drawMode) :
    m_name(_name),
    m_shaderProgram(std::make_unique<ShaderProgram>()),
    m_material(std::make_shared<Material>()),
    m_blend(_blendMode),
    m_drawMode(_drawMode)
{}

Style::~Style() {}

void Style::build(const std::vector<std::unique_ptr<Light>>& _lights) {

    constructVertexLayout();
    constructShaderProgram();

    switch (m_lightingType) {
        case LightingType::vertex:
            m_shaderProgram->addSourceBlock("defines", "#define TANGRAM_LIGHTING_VERTEX\n", false);
            break;
        case LightingType::fragment:
            m_shaderProgram->addSourceBlock("defines", "#define TANGRAM_LIGHTING_FRAGMENT\n", false);
            break;
        default:
            break;
    }

    m_material->injectOnProgram(*m_shaderProgram);

    for (auto& light : _lights) {
        light->injectOnProgram(*m_shaderProgram);
    }

}

void Style::setMaterial(const std::shared_ptr<Material>& _material) {

    m_material = _material;

}

void Style::setLightingType(LightingType _type){

    m_lightingType = _type;

}

bool Style::checkRule(const DrawRule& _rule) const {

    uint32_t checkColor;
    uint32_t checkOrder;

    if (!_rule.get(StyleParamKey::color, checkColor)) {
        const auto& blocks = m_shaderProgram->getSourceBlocks();
        if (blocks.find("color") == blocks.end() && blocks.find("filter") == blocks.end()) {
            return false; // No color parameter or color block? NO SOUP FOR YOU
        }
    }

    if (!_rule.get(StyleParamKey::order, checkOrder)) {
        return false;
    }

    return true;
}

void Style::buildFeature(Tile& _tile, const Feature& _feat, const DrawRule& _rule) const {

    if (!checkRule(_rule)) { return; }

    bool visible;
    if (_rule.get(StyleParamKey::visible, visible) && !visible) {
        return;
    }

    auto& mesh = _tile.getMesh(*this);

    if (!mesh) {
        mesh.reset(newMesh());
    }

    switch (_feat.geometryType) {
        case GeometryType::points:
            for (auto& point : _feat.points) {
                buildPoint(point, _rule, _feat.props, *mesh, _tile);
            }
            break;
        case GeometryType::lines:
            for (auto& line : _feat.lines) {
                buildLine(line, _rule, _feat.props, *mesh, _tile);
            }
            break;
        case GeometryType::polygons:
            for (auto& polygon : _feat.polygons) {
                buildPolygon(polygon, _rule, _feat.props, *mesh, _tile);
            }
            break;
        default:
            break;
    }

}

void Style::setupShaderUniforms(int _textureUnit, Scene& _scene) {
    for (const auto& uniformPair : m_styleUniforms) {
        const auto& name = uniformPair.first;
        const auto& value = uniformPair.second;

        auto& textures = _scene.textures();

        if (value.is<std::string>()) {

            auto& tex = textures[value.get<std::string>()];

            tex->update(_textureUnit);
            tex->bind(_textureUnit);

            m_shaderProgram->setUniformi(name, _textureUnit);

            _textureUnit++;

        } else {

            if (value.is<bool>()) {
                m_shaderProgram->setUniformi(name, value.get<bool>());
            } else if(value.is<float>()) {
                m_shaderProgram->setUniformf(name, value.get<float>());
            } else if(value.is<glm::vec2>()) {
                m_shaderProgram->setUniformf(name, value.get<glm::vec2>());
            } else if(value.is<glm::vec3>()) {
                m_shaderProgram->setUniformf(name, value.get<glm::vec3>());
            } else if(value.is<glm::vec4>()) {
                m_shaderProgram->setUniformf(name, value.get<glm::vec4>());
            } else {
                // TODO: Throw away uniform on loading!
                // none_type
            }
        }
    }
}

void Style::onBeginDrawFrame(const View& _view, Scene& _scene, int _textureUnit) {

    m_shaderProgram->setUniformf("u_device_pixel_ratio", m_pixelScale);

    m_material->setupProgram(*m_shaderProgram);

    // Set up lights
    for (const auto& light : _scene.lights()) {
        light->setupProgram(_view, *m_shaderProgram);
    }

    // Set Map Position
    m_shaderProgram->setUniformf("u_resolution", _view.getWidth(), _view.getHeight());

    const auto& mapPos = _view.getPosition();
    m_shaderProgram->setUniformf("u_map_position", mapPos.x, mapPos.y, _view.getZoom());
    m_shaderProgram->setUniformMatrix3f("u_normalMatrix", _view.getNormalMatrix());
    m_shaderProgram->setUniformMatrix3f("u_inverseNormalMatrix", glm::inverse(_view.getNormalMatrix()));
    m_shaderProgram->setUniformf("u_meters_per_pixel", 1.0 / _view.pixelsPerMeter());
    m_shaderProgram->setUniformMatrix4f("u_view", _view.getViewMatrix());
    m_shaderProgram->setUniformMatrix4f("u_proj", _view.getProjectionMatrix());

    setupShaderUniforms(_textureUnit, _scene);

    // Configure render state
    switch (m_blend) {
        case Blending::none:
            RenderState::blending(GL_FALSE);
            RenderState::blendingFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            RenderState::depthTest(GL_TRUE);
            RenderState::depthWrite(GL_TRUE);
            break;
        case Blending::add:
            RenderState::blending(GL_TRUE);
            RenderState::blendingFunc(GL_ONE, GL_ONE);
            RenderState::depthTest(GL_FALSE);
            RenderState::depthWrite(GL_TRUE);
            break;
        case Blending::multiply:
            RenderState::blending(GL_TRUE);
            RenderState::blendingFunc(GL_ZERO, GL_SRC_COLOR);
            RenderState::depthTest(GL_FALSE);
            RenderState::depthWrite(GL_TRUE);
            break;
        case Blending::overlay:
            RenderState::blending(GL_TRUE);
            RenderState::blendingFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            RenderState::depthTest(GL_FALSE);
            RenderState::depthWrite(GL_FALSE);
            break;
        case Blending::inlay:
            // TODO: inlay does not behave correctly for labels because they don't have a z position
            RenderState::blending(GL_TRUE);
            RenderState::blendingFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            RenderState::depthTest(GL_TRUE);
            RenderState::depthWrite(GL_FALSE);
            break;
        default:
            break;
    }
}

void Style::onBeginBuildTile(Tile& _tile) const {
    // No-op by default
}

void Style::onEndBuildTile(Tile& _tile) const {
    // No-op by default
}

void Style::buildPoint(const Point& _point, const DrawRule& _rule, const Properties& _props, VboMesh& _mesh, Tile& _tile) const {
    // No-op by default
}

void Style::buildLine(const Line& _line, const DrawRule& _rule, const Properties& _props, VboMesh& _mesh, Tile& _tile) const {
    // No-op by default
}

void Style::buildPolygon(const Polygon& _polygon, const DrawRule& _rule, const Properties& _props, VboMesh& _mesh, Tile& _tile) const {
    // No-op by default
}

}
