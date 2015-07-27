#include "style.h"

#include "scene/scene.h"
#include "scene/sceneLayer.h"
#include "scene/light.h"
#include "tile/tile.h"
#include "gl/vboMesh.h"
#include "view/view.h"
#include "csscolorparser.hpp"
#include "geom.h" // for CLAMP

#include <sstream>

namespace Tangram {

std::unordered_map<Style::StyleCacheKey, StyleParamMap> Style::s_styleParamMapCache;
std::mutex Style::s_cacheMutex;

using namespace Tangram;

Style::Style(std::string _name, GLenum _drawMode) : m_name(_name), m_drawMode(_drawMode) {
}

Style::~Style() {
    m_layers.clear();
}

uint32_t Style::parseColorProp(const std::string& _colorPropStr) {
    uint32_t color = 0;

    if (isdigit(_colorPropStr.front())) {
        // try to parse as comma-separated rgba components
        float r, g, b;
        if (sscanf(_colorPropStr.c_str(), "%f,%f,%f", &r, &g, &b) == 3) {
            color = 0xff000000
                | (CLAMP(static_cast<uint32_t>(r * 255.), 0, 255)) << 16
                | (CLAMP(static_cast<uint32_t>(g * 255.), 0, 255)) << 8
                | (CLAMP(static_cast<uint32_t>(b * 255.), 0, 255));

        } else {
            color = 0xffff00ff;
        }
    } else { // parse as css color or #hex-num
        bool isValid;
        color = CSSColorParser::parse(_colorPropStr, isValid).getInt();
        if (!isValid) {
            color = 0xffff00ff;
        }
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

void Style::applyLayerFiltering(const Feature& _feature, const Context& _ctx, StyleCacheKey& _uniqueID,
                                   StyleParamMap& _styleParamMapMix, std::shared_ptr<SceneLayer> _uberLayer) const {

    std::vector<std::shared_ptr<SceneLayer>> sLayers;
    sLayers.reserve(_uberLayer->getSublayers().size() + 1);
    sLayers.push_back(_uberLayer);

    auto sLayerItr = sLayers.begin();

    // A BFS traversal of the SceneLayer graph
    while (sLayerItr != sLayers.end()) {

        auto sceneLyr = *sLayerItr;

        if (sceneLyr->getFilter().eval(_feature, _ctx)) { // filter matches

            _uniqueID.set(sceneLyr->getID());
            {
                std::lock_guard<std::mutex> lock(s_cacheMutex);

                // Get or create cache entry
                auto& entry = s_styleParamMapCache[_uniqueID];

                if (!entry.empty()) {
                    _styleParamMapMix = entry;

                } else {
                    // Update StyleParam with subLayer parameters

                    auto& layerStyleParamMap = sceneLyr->getStyleParamMap();
                    for(auto& styleParam : layerStyleParamMap) {
                        _styleParamMapMix[styleParam.first] = styleParam.second;
                    }
                    entry = _styleParamMapMix;
                }
            }

            // Append sLayers with sublayers of this layer
            auto& ssLayers = sceneLyr->getSublayers();
            sLayerItr = sLayers.insert(sLayers.end(), ssLayers.begin(), ssLayers.end());
        } else {
            sLayerItr++;
        }
    }
}

void Style::addData(TileData& _data, Tile& _tile) {

    std::shared_ptr<VboMesh> mesh(newMesh());
    
    onBeginBuildTile(*mesh);

    Context ctx;
    ctx["$zoom"] = Value(_tile.getID().z);

    for (auto& layer : _data.layers) {

        // Skip any layers that this style doesn't have a rule for
        auto it = m_layers.begin();
        while (it != m_layers.end() && (*it)->getName() != layer.name) { ++it; }
        if (it == m_layers.end()) { continue; }

        // Loop over all features
        for (auto& feature : layer.features) {

            StyleCacheKey uniqueID(0);
            StyleParamMap styleParamMapMix;
            applyLayerFiltering(feature, ctx, uniqueID, styleParamMapMix, (*it));

            if(uniqueID.any()) { // if a layer matched then uniqueID should be > 0

                switch (feature.geometryType) {
                    case GeometryType::points:
                        // Build points
                        for (auto& point : feature.points) {
                            buildPoint(point, styleParamMapMix, feature.props, *mesh, _tile);
                        }
                        break;
                    case GeometryType::lines:
                        // Build lines
                        for (auto& line : feature.lines) {
                            buildLine(line, styleParamMapMix, feature.props, *mesh, _tile);
                        }
                        break;
                    case GeometryType::polygons:
                        // Build polygons
                        for (auto& polygon : feature.polygons) {
                            buildPolygon(polygon, styleParamMapMix, feature.props, *mesh, _tile);
                        }
                        break;
                    default:
                        break;
                }
            }
        }
    }

    onEndBuildTile(*mesh);

    if (mesh->numVertices() == 0) {
        mesh.reset();
    } else {
        mesh->compileVertexBuffer();

        _tile.addMesh(*this, mesh);
    }
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

void Style::onBeginBuildTile(VboMesh& _mesh) const {
    // No-op by default
}

void Style::onEndBuildTile(VboMesh& _mesh) const {
    // No-op by default
}

void Style::buildPoint(Point& _point, const StyleParamMap& _styleParamMap, Properties& _props, VboMesh& _mesh, Tile& _tile) const {
    // No-op by default
}

void Style::buildLine(Line& _line, const StyleParamMap& _styleParamMap, Properties& _props, VboMesh& _mesh, Tile& _tile) const {
    // No-op by default
}

void Style::buildPolygon(Polygon& _polygon, const StyleParamMap& _styleParamMap, Properties& _props, VboMesh& _mesh, Tile& _tile) const {
    // No-op by default
}

}
