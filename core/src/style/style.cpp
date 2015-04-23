#include "style.h"
#include "scene/scene.h"

/*
 * Style Class Methods
 */

Style::Style(std::string _name, GLenum _drawMode) : m_name(_name), m_drawMode(_drawMode) {
}

Style::~Style() {
    m_layers.clear();
}

void Style::addLayers(std::vector<std::string> _layers) {
    m_layers.insert(_layers.cbegin(), _layers.cend());
}

void Style::addData(TileData& _data, MapTile& _tile, const MapProjection& _mapProjection)  const {
    prepareDataProcessing(_tile);

    VboMesh* mesh = newMesh();
    
    for (auto& layer : _data.layers) {
        
        if (m_layers.find(layer.name) == m_layers.end()) {
            continue;
        }
        
        for (auto& feature : layer.features) {

            feature.props.numericProps["zoom"] = _tile.getID().z;
            
            switch (feature.geometryType) {
                case GeometryType::POINTS:
                    // Build points
                    for (auto& point : feature.points) {
                        buildPoint(point, layer.name, feature.props, *mesh);
                    }
                    break;
                case GeometryType::LINES:
                    // Build lines
                    for (auto& line : feature.lines) {
                        buildLine(line, layer.name, feature.props, *mesh);
                    }
                    break;
                case GeometryType::POLYGONS:
                    // Build polygons
                    for (auto& polygon : feature.polygons) {
                        buildPolygon(polygon, layer.name, feature.props, *mesh);
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
    finishDataProcessing(_tile);
}

void Style::setupFrame(const std::shared_ptr<View>& _view, const std::shared_ptr<Scene>& _scene) {
    // Set up material
    m_material.setupProgram(m_shaderProgram);
    
    // Set up lights
    for (const auto& light : _scene->getLights()) {
        light.second->setupProgram(m_shaderProgram);
    }
    
    m_shaderProgram->setUniformf("u_zoom", _view->getZoom());
}

void Style::setLighting( LightingType _lType ){

    if ( _lType == LIGHTING_VERTEX ) {
        m_shaderProgram->delSourceBlock("defines", "#define TANGRAM_LIGHTING_FRAGMENT\n");
        m_shaderProgram->addSourceBlock("defines", "#define TANGRAM_LIGHTING_VERTEX\n", false);
    } else if  (_lType == LIGHTING_FRAGMENT ) {
        m_shaderProgram->delSourceBlock("defines", "#define TANGRAM_LIGHTING_VERTEX\n");
        m_shaderProgram->addSourceBlock("defines", "#define TANGRAM_LIGHTING_FRAGMENT\n", false);
    } else {
        m_shaderProgram->delSourceBlock("defines", "#define TANGRAM_LIGHTING_VERTEX\n");
        m_shaderProgram->delSourceBlock("defines", "#define TANGRAM_LIGHTING_FRAGMENT\n");
    }
    
}
    
void Style::setupTile(const std::shared_ptr<MapTile>& _tile) {
    // No-op by default
}

void Style::prepareDataProcessing(MapTile& _tile) const {
    // No-op by default
}

void Style::finishDataProcessing(MapTile& _tile) const {
    // No-op by default
}
