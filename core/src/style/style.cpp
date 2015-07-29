#include "style.h"

#include "scene/scene.h"
#include "scene/sceneLayer.h"
#include "scene/light.h"
#include "tile/tile.h"
#include "gl/vboMesh.h"
#include "view/view.h"

#include "csscolorparser.hpp"

#include <sstream>

namespace Tangram {

Style::Style(std::string _name, GLenum _drawMode) : m_name(_name), m_drawMode(_drawMode) {
}

Style::~Style() {}

uint32_t Style::parseColorProp(const std::string& _colorPropStr) {
    uint32_t color = 0;

    if (_colorPropStr.find(',') != std::string::npos) { // try to parse as comma-separated rgba components
        std::istringstream stream(_colorPropStr);
        std::string token;
        unsigned char i = 0;
        while (std::getline(stream, token, ',') && i < 4) {
            color += (uint32_t(std::stod(token) * 255.)) << (8 * i++);
        }
    } else { // parse as css color or #hex-num
        color = CSSColorParser::parse(_colorPropStr).getInt();
    }
    return color;
}

void Style::build(const std::vector<std::shared_ptr<Light>>& _lights) {

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

    m_material->injectOnProgram(m_shaderProgram);

    for (auto& light : _lights) {
        light->injectOnProgram(m_shaderProgram);
    }

}

void Style::setMaterial(const std::shared_ptr<Material>& _material) {

    m_material = _material;

}

void Style::setLightingType(LightingType _type){

    m_lightingType = _type;

}

void Style::addLayer(std::shared_ptr<SceneLayer> _layer) {

    m_layers.push_back(std::move(_layer));

}

void Style::buildFeature(Tile& _tile, const Feature& _feat, const DrawRule& _rule) const {

    auto& mesh = _tile.getGeometry(*this);

    if (!mesh) {
        mesh.reset(newMesh());
    }

    switch (_feat.geometryType) {
        case GeometryType::points:
            for (auto& point : _feat.points) {
                buildPoint(point, _rule.parameters, _feat.props, *mesh, _tile);
            }
            break;
        case GeometryType::lines:
            for (auto& line : _feat.lines) {
                buildLine(line, _rule.parameters, _feat.props, *mesh, _tile);
            }
            break;
        case GeometryType::polygons:
            for (auto& polygon : _feat.polygons) {
                buildPolygon(polygon, _rule.parameters, _feat.props, *mesh, _tile);
            }
            break;
        default:
            break;
    }

}

void Style::onBeginDrawFrame(const std::shared_ptr<View>& _view, const std::shared_ptr<Scene>& _scene) {

    m_material->setupProgram(m_shaderProgram);

    // Set up lights
    for (const auto& light : _scene->lights()) {
        light->setupProgram(_view, m_shaderProgram);
    }

    m_shaderProgram->setUniformf("u_zoom", _view->getZoom());
    
    // default capabilities
    RenderState::blending(GL_FALSE);
    RenderState::depthTest(GL_TRUE);
}

void Style::onBeginBuildTile(Tile& _tile) const {
    // No-op by default
}

void Style::onEndBuildTile(Tile& _tile) const {
    // No-op by default
}

void Style::buildPoint(const Point& _point, const StyleParamMap& _styleParamMap, const Properties& _props, VboMesh& _mesh, Tile& _tile) const {
    // No-op by default
}

void Style::buildLine(const Line& _line, const StyleParamMap& _styleParamMap, const Properties& _props, VboMesh& _mesh, Tile& _tile) const {
    // No-op by default
}

void Style::buildPolygon(const Polygon& _polygon, const StyleParamMap& _styleParamMap, const Properties& _props, VboMesh& _mesh, Tile& _tile) const {
    // No-op by default
}

}
