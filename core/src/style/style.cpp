#include "style.h"

#include "scene/scene.h"
#include "scene/sceneLayer.h"
#include "scene/light.h"
#include "tile/tile.h"
#include "gl/vboMesh.h"
#include "view/view.h"

namespace Tangram {

Style::Style(std::string _name, GLenum _drawMode) :
    m_name(_name),
    m_drawMode(_drawMode),
    m_contextLost(true) {
}

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

void Style::buildFeature(Tile& _tile, const Feature& _feat, const DrawRule& _rule) const {

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

void Style::onBeginDrawFrame(const View& _view, const Scene& _scene) {

    m_material->setupProgram(*m_shaderProgram);

    // Set up lights
    for (const auto& light : _scene.lights()) {
        light->setupProgram(_view, *m_shaderProgram);
    }

    m_shaderProgram->setUniformf("u_zoom", _view.getZoom());
    
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

void Style::buildPoint(const Point& _point, const DrawRule& _rule, const Properties& _props, VboMesh& _mesh, Tile& _tile) const {
    // No-op by default
}

void Style::buildLine(const Line& _line, const DrawRule& _rule, const Properties& _props, VboMesh& _mesh, Tile& _tile) const {
    // No-op by default
}

void Style::buildPolygon(const Polygon& _polygon, const DrawRule& _rule, const Properties& _props, VboMesh& _mesh, Tile& _tile) const {
    // No-op by default
}
    
bool Style::glContextLost() {
    bool contextLost = m_contextLost;
    if (m_contextLost) {
        m_contextLost = false;
    }
    return contextLost;
}

}
