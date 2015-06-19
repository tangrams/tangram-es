#include "style.h"
#include "scene/scene.h"
#include "util/vboMesh.h"

using namespace CSSColorParser;

Style::Style(std::string _name, GLenum _drawMode) : m_name(_name), m_drawMode(_drawMode) {
}

Style::~Style() {
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

void Style::addLayer(const std::pair<std::string, StyleParamMap>&& _layer) {

    m_layers.push_back(std::move(_layer));

}

void Style::addData(TileData& _data, MapTile& _tile, const MapProjection& _mapProjection) {
    onBeginBuildTile(_tile);

    VboMesh* mesh = newMesh();

    for (auto& layer : _data.layers) {

        // Skip any layers that this style doesn't have a rule for
        auto it = m_layers.begin();
        while (it != m_layers.end() && it->first != layer.name) { ++it; }
        if (it == m_layers.end()) { continue; }

        // Loop over all features
        for (auto& feature : layer.features) {

            /*
             * TODO: do filter evaluation for each feature for sublayer!
             *     construct a unique ID for a the set of filters matched
             *     use this ID pass to the style's parseStyleParams method to construct styleParam cache
             *     NOTE: for the time being use layerName as ID for cache
             */

            feature.props.numericProps["zoom"] = _tile.getID().z;

            switch (feature.geometryType) {
                case GeometryType::POINTS:
                    // Build points
                    for (auto& point : feature.points) {
                        buildPoint(point, parseStyleParams(it->first, it->second), feature.props, *mesh);
                    }
                    break;
                case GeometryType::LINES:
                    // Build lines
                    for (auto& line : feature.lines) {
                        buildLine(line, parseStyleParams(it->first, it->second), feature.props, *mesh);
                    }
                    break;
                case GeometryType::POLYGONS:
                    // Build polygons
                    for (auto& polygon : feature.polygons) {
                        buildPolygon(polygon, parseStyleParams(it->first, it->second), feature.props, *mesh);
                    }
                    break;
                default:
                    break;
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
