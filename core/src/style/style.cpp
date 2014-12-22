#include "style.h"

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

void Style::addData(TileData& _data, MapTile& _tile, const MapProjection& _mapProjection) {
    onTileFetched(_tile);

    VboMesh* mesh = new VboMesh(m_vertexLayout, m_drawMode);
    
    for (auto& layer : _data.layers) {
        
        if (m_layers.find(layer.name) == m_layers.end()) {
            continue;
        }
        
        for (auto& feature : layer.features) {

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
    
    _tile.addGeometry(*this, std::unique_ptr<VboMesh>(mesh));
    
}
