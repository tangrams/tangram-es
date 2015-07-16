#include "style.h"

#include "scene/scene.h"
#include "scene/sceneLayer.h"
#include "scene/light.h"
#include "tile/mapTile.h"
#include "util/vboMesh.h"
#include "view/view.h"

#include "csscolorparser.hpp"

#include <sstream>

std::unordered_map<std::bitset<MAX_LAYERS>, StyleParamMap> Style::s_styleParamMapCache;
std::mutex Style::s_cacheMutex;

using namespace Tangram;

Style::Style(std::string _name, GLenum _drawMode) : m_name(_name), m_drawMode(_drawMode) {
}

Style::~Style() {
    m_layers.clear();
}

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

void Style::applyLayerFiltering(const Feature& _feature, const Context& _ctx, std::bitset<MAX_LAYERS>& _uniqueID,
                                   StyleParamMap& _styleParamMapMix, std::shared_ptr<SceneLayer> _uberLayer) const {

    std::vector<std::shared_ptr<SceneLayer>> sLayers;
    sLayers.reserve(_uberLayer->getSublayers().size() + 1);
    sLayers.push_back(_uberLayer);

    auto sLayerItr = sLayers.begin();

    //A BFS traversal of the SceneLayer graph
    while (sLayerItr != sLayers.end()) {

        auto sceneLyr = *sLayerItr;

        if ( sceneLyr->getFilter()->eval(_feature, _ctx)) { // filter matches

            _uniqueID.set(sceneLyr->getID());

            {
                std::lock_guard<std::mutex> lock(s_cacheMutex);

                if(s_styleParamMapCache.find(_uniqueID) != s_styleParamMapCache.end()) {

                    _styleParamMapMix = s_styleParamMapCache.at(_uniqueID);

                } else {

                    /* update StyleParam with subLayer parameters */
                    auto& layerStyleParamMap = sceneLyr->getStyleParamMap();
                    for(auto& styleParam : layerStyleParamMap) {
                        _styleParamMapMix[styleParam.first] = styleParam.second;
                    }

                    s_styleParamMapCache.emplace(_uniqueID, _styleParamMapMix);
                }
            }

            /* append sLayers with sublayers of this layer */
            auto& ssLayers = sceneLyr->getSublayers();
            sLayerItr = sLayers.insert(sLayers.end(), ssLayers.begin(), ssLayers.end());
        } else {
            sLayerItr++;
        }
    }
}

void Style::addData(TileData& _data, MapTile& _tile) {
    onBeginBuildTile(_tile);

    std::shared_ptr<VboMesh> mesh(newMesh());

    Context ctx;
    auto ctxVal = new NumValue(_tile.getID().z);
    ctx["$zoom"] = ctxVal;

    for (auto& layer : _data.layers) {

        // Skip any layers that this style doesn't have a rule for
        auto it = m_layers.begin();
        while (it != m_layers.end() && (*it)->getName() != layer.name) { ++it; }
        if (it == m_layers.end()) { continue; }

        // Loop over all features
        for (auto& feature : layer.features) {

            std::bitset<MAX_LAYERS> uniqueID(0);
            StyleParamMap styleParamMapMix;
            applyLayerFiltering(feature, ctx, uniqueID, styleParamMapMix, (*it));

            if(uniqueID.any()) { // if a layer matched then uniqueID should be > 0
                feature.props.numericProps["zoom"] = _tile.getID().z;

                switch (feature.geometryType) {
                    case GeometryType::points:
                        // Build points
                        for (auto& point : feature.points) {
                            buildPoint(point, styleParamMapMix, feature.props, *mesh);
                        }
                        break;
                    case GeometryType::lines:
                        // Build lines
                        for (auto& line : feature.lines) {
                            buildLine(line, styleParamMapMix, feature.props, *mesh);
                        }
                        break;
                    case GeometryType::polygons:
                        // Build polygons
                        for (auto& polygon : feature.polygons) {
                            buildPolygon(polygon, styleParamMapMix, feature.props, *mesh);
                        }
                        break;
                    default:
                        break;
                }
            }
        }
    }

    onEndBuildTile(_tile, mesh);

    if (mesh->numVertices() == 0) {
        mesh.reset();
    } else {
        mesh->compileVertexBuffer();

        _tile.addGeometry(*this, mesh);
    }

    delete ctxVal;
}

void Style::onBeginDrawFrame(const std::shared_ptr<View>& _view, const std::shared_ptr<Scene>& _scene) {

    m_material->setupProgram(m_shaderProgram);

    // Set up lights
    for (const auto& light : _scene->getLights()) {
        light->setupProgram(_view, m_shaderProgram);
    }

    m_shaderProgram->setUniformf("u_zoom", _view->getZoom());
    
    // default capabilities
    RenderState::blending(GL_FALSE);
    RenderState::depthTest(GL_TRUE);
}

void Style::onBeginBuildTile(MapTile& _tile) const {
    // No-op by default
}

void Style::onEndBuildTile(MapTile& _tile, std::shared_ptr<VboMesh> _mesh) const {
    // No-op by default
}
