#include "style.h"
#include "scene/scene.h"
#include "scene/sceneLayer.h"
#include "util/vboMesh.h"

std::unordered_map<long long, StyleParamMap> Style::s_styleParamMapCache;
std::mutex Style::s_cacheMutex;

using namespace CSSColorParser;
using namespace Tangram;

Style::Style(std::string _name, GLenum _drawMode) : m_name(_name), m_drawMode(_drawMode) {
}

Style::~Style() {
    for(auto& layer : m_layers) {
        delete layer;
    }
    m_layers.clear();
}

uint32_t Style::parseColorProp(std::string _colorPropStr) {
    uint32_t color = 0;
    if(std::isdigit(_colorPropStr[0])) { // r, g, b, a
        int shift = 0;
        size_t start = 0;
        auto pos = _colorPropStr.find_first_of(",", start);
        while(pos != std::string::npos) {
            if(pos != start) {
                std::string value(_colorPropStr, start, pos - start);
                color += (static_cast<uint32_t>(255.0 * std::stof(value)) << shift);
                shift += 8;
            }
            start = pos + 1;
            pos = _colorPropStr.find_first_of(",", start);
        }
        if(start < _colorPropStr.length()) {
            std::string value(_colorPropStr, start, pos - start);
            color += (static_cast<uint32_t>(255.0 * std::stof(value)) << shift);
            shift += 8;
        }
        if(shift == 24) {
            color += (255 << 24);
        }
    } else { // css color or #hex-num
        color = parse(_colorPropStr).getInt();
    }
    return color;
}

void Style::setMaterial(const std::shared_ptr<Material>& _material) {

    if ( m_material ) {
        m_material->removeFromProgram(m_shaderProgram);
    }

    m_material = _material;
    m_material->injectOnProgram(m_shaderProgram);

}

void Style::addLayer(SceneLayer* _layer) {

    m_layers.push_back(_layer);

}

void Style::applyLayerFiltering(const Feature& _feature, const Context& _ctx, long long& _uniqueID,
                                   StyleParamMap& _styleParamMapMix, SceneLayer* _uberLayer) const {

    std::vector<SceneLayer*> sLayers;
    sLayers.reserve(_uberLayer->getSublayers().size() + 1);
    sLayers.push_back(_uberLayer);

    auto sLayerItr = sLayers.begin();

    //A BFS traversal of the SceneLayer graph
    while(sLayerItr != sLayers.end()) {
        auto layerFilter = (*sLayerItr)->getFilter();
        if( (*sLayerItr)->getFilter()->eval(_feature, _ctx)) { // filter matches
            _uniqueID += (1 << (*sLayerItr)->getID());

            if(s_styleParamMapCache.find(_uniqueID) != s_styleParamMapCache.end()) {
                _styleParamMapMix = s_styleParamMapCache.at(_uniqueID);
            } else {
                /* update StyleParam with subLayer parameters */
                auto& sLayerStyleParamMap = (*sLayerItr)->getStyleParamMap();
                for(auto& styleParam : sLayerStyleParamMap) {
                    auto it = _styleParamMapMix.find(styleParam.first);
                    if(it != _styleParamMapMix.end()) {
                        it->second = styleParam.second;
                    } else {
                        _styleParamMapMix.insert(styleParam);
                    }
                }

                {
                    std::lock_guard<std::mutex> lock(s_cacheMutex);
                    s_styleParamMapCache.emplace(_uniqueID, _styleParamMapMix);
                }
            }

            /* append sLayers with sublayers of this layer */
            auto& ssLayers = (*sLayerItr)->getSublayers();
            sLayers.reserve(sLayers.size() + ssLayers.size());
            sLayerItr = sLayers.insert(sLayers.end(), ssLayers.begin(), ssLayers.end());
        } else {
            sLayerItr++;
        }
    }
}

void Style::addData(TileData& _data, MapTile& _tile, const MapProjection& _mapProjection) {
    onBeginBuildTile(_tile);

    VboMesh* mesh = newMesh();

    Context ctx;
    ctx["$zoom"] = new NumValue(_tile.getID().z);

    for (auto& layer : _data.layers) {

        // Skip any layers that this style doesn't have a rule for
        auto it = m_layers.begin();
        while (it != m_layers.end() && (*it)->getName() != layer.name) { ++it; }
        if (it == m_layers.end()) { continue; }

        // Loop over all features
        for (auto& feature : layer.features) {

            // NOTE: Makes a restriction on number of layers in the style confic (64 max)
            long long uniqueID = 0;
            StyleParamMap styleParamMapMix;
            applyLayerFiltering(feature, ctx, uniqueID, styleParamMapMix, (*it));

            if(uniqueID != 0) { // if a layer matched then uniqueID should be > 0
                feature.props.numericProps["zoom"] = _tile.getID().z;

                switch (feature.geometryType) {
                    case GeometryType::POINTS:
                        // Build points
                        for (auto& point : feature.points) {
                            buildPoint(point, parseStyleParams(styleParamMapMix), feature.props, *mesh);
                        }
                        break;
                    case GeometryType::LINES:
                        // Build lines
                        for (auto& line : feature.lines) {
                            buildLine(line, parseStyleParams(styleParamMapMix), feature.props, *mesh);
                        }
                        break;
                    case GeometryType::POLYGONS:
                        // Build polygons
                        for (auto& polygon : feature.polygons) {
                            buildPolygon(polygon, parseStyleParams(styleParamMapMix), feature.props, *mesh);
                        }
                        break;
                    default:
                        break;
                }
            }
        }
    }

    if (mesh->numVertices() == 0) {
        delete mesh;
    } else {
        mesh->compileVertexBuffer();

        _tile.addGeometry(*this, std::unique_ptr<VboMesh>(mesh));
    }
    onEndBuildTile(_tile);
}

void Style::onBeginDrawFrame(const std::shared_ptr<View>& _view, const std::shared_ptr<Scene>& _scene) {

    // Set up material
    if (!m_material) {
        setMaterial(std::make_shared<Material>());
    }

    m_material->setupProgram(m_shaderProgram);

    // Set up lights
    for (const auto& light : _scene->getLights()) {
        light.second->setupProgram(_view, m_shaderProgram);
    }

    m_shaderProgram->setUniformf("u_zoom", _view->getZoom());

}

void Style::setLightingType(LightingType _lType){

    if ( _lType == LightingType::vertex ) {
        m_shaderProgram->removeSourceBlock("defines", "#define TANGRAM_LIGHTING_FRAGMENT\n");
        m_shaderProgram->addSourceBlock(   "defines", "#define TANGRAM_LIGHTING_VERTEX\n", false);
    } else if  (_lType == LightingType::fragment ) {
        m_shaderProgram->removeSourceBlock("defines", "#define TANGRAM_LIGHTING_VERTEX\n");
        m_shaderProgram->addSourceBlock(   "defines", "#define TANGRAM_LIGHTING_FRAGMENT\n", false);
    } else {
        m_shaderProgram->removeSourceBlock("defines", "#define TANGRAM_LIGHTING_VERTEX\n");
        m_shaderProgram->removeSourceBlock("defines", "#define TANGRAM_LIGHTING_FRAGMENT\n");
    }

}

void Style::onBeginDrawTile(const std::shared_ptr<MapTile>& _tile) {
    // No-op by default
}

void Style::onBeginBuildTile(MapTile& _tile) const {
    // No-op by default
}

void Style::onEndBuildTile(MapTile& _tile) const {
    // No-op by default
}
