#include "mapTile.h"
#include "style/style.h"
#include "view/view.h"
#include "util/tileID.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

MapTile::MapTile(TileID _id, const MapProjection& _projection) : m_id(_id),  m_projection(&_projection) {

    glm::dvec4 bounds = _projection.TileBounds(_id); // [x: xmin, y: ymin, z: xmax, w: ymax]
    
    m_scale = 0.5 * glm::abs(bounds.x - bounds.z);
    m_inverseScale = 1.0/m_scale;
    
    // negative y coordinate: to change from y down to y up (tile system has y down and gl context we use has y up).
    m_tileOrigin = glm::dvec2(0.5*(bounds.x + bounds.z), -0.5*(bounds.y + bounds.w));
    
    m_modelMatrix = glm::mat4(1.0);
    
    // Scale model matrix to size of tile
    m_modelMatrix = glm::scale(m_modelMatrix, glm::vec3(m_scale));

}

MapTile::MapTile(MapTile&& _other) : m_id(std::move(m_id)), m_proxyCounter(std::move(_other.m_proxyCounter)), 
                                     m_projection(std::move(_other.m_projection)), m_scale(std::move(_other.m_scale)), 
                                     m_inverseScale(std::move(_other.m_inverseScale)), m_tileOrigin(std::move(_other.m_tileOrigin)), 
                                     m_modelMatrix(std::move(_other.m_modelMatrix)), m_geometry(std::move(_other.m_geometry)), 
                                     m_buffers(std::move(_other.m_buffers)) {}


MapTile::~MapTile() {

    m_geometry.clear();
    m_buffers.clear();

}

void MapTile::addGeometry(const Style& _style, std::unique_ptr<VboMesh> _mesh) {

    m_geometry[_style.getName()] = std::move(_mesh); // Move-construct a unique_ptr at the value associated with the given style

}

void MapTile::setTextBuffer(const Style& _style, std::shared_ptr<TextBuffer> _buffer) {

    m_buffers[_style.getName()] = _buffer;
}

std::shared_ptr<TextBuffer> MapTile::getTextBuffer(const Style& _style) const {
    auto it = m_buffers.find(_style.getName());

    if (it != m_buffers.end()) {
        return it->second;
    }

    return nullptr;
}

void MapTile::update(float _dt, const View& _view) {

    // Apply tile-view translation to the model matrix
    const auto& viewOrigin = _view.getPosition();
    m_modelMatrix[3][0] = m_tileOrigin.x - viewOrigin.x;
    m_modelMatrix[3][1] = m_tileOrigin.y - viewOrigin.y;
    m_modelMatrix[3][2] = -viewOrigin.z;

}

void MapTile::updateLabels(float _dt, const Style& _style, const View& _view, std::shared_ptr<LabelContainer> _labelContainer) {
    glm::mat4 mvp = _view.getViewProjectionMatrix() * m_modelMatrix;
    glm::vec2 screenSize = glm::vec2(_view.getWidth(), _view.getHeight());
    
    for(auto& label : _labelContainer->getLabels(_style.getName(), getID())) {
        label->update(mvp, screenSize, _dt);
    }
}

void MapTile::pushLabelTransforms(const Style& _style, std::shared_ptr<LabelContainer> _labelContainer) {

    if(m_buffers[_style.getName()]) {
        auto ftContext = _labelContainer->getFontContext();

        ftContext->lock();
        
        for(auto& label : _labelContainer->getLabels(_style.getName(), getID())) {
            label->pushTransform();
        }
        
        m_buffers[_style.getName()]->triggerTransformUpdate();
        
        ftContext->unlock();
    }
    
}

void MapTile::draw(const Style& _style, const View& _view) {

    const std::unique_ptr<VboMesh>& styleMesh = m_geometry[_style.getName()];
    
    if (styleMesh) {
        
        std::shared_ptr<ShaderProgram> shader = _style.getShaderProgram();

        glm::mat4 modelViewMatrix = _view.getViewMatrix() * m_modelMatrix;
        glm::mat4 modelViewProjMatrix = _view.getViewProjectionMatrix() * m_modelMatrix;
        
        shader->setUniformMatrix4f("u_modelView", glm::value_ptr(modelViewMatrix));
        shader->setUniformMatrix4f("u_modelViewProj", glm::value_ptr(modelViewProjMatrix));
        shader->setUniformMatrix3f("u_normalMatrix", glm::value_ptr(_view.getNormalMatrix()));

        // Set the tile zoom level, using the sign to indicate whether the tile is a proxy
        shader->setUniformf("u_tile_zoom", m_proxyCounter > 0 ? -m_id.z : m_id.z);

        styleMesh->draw(shader);
    }
}

bool MapTile::hasGeometry() {
    return (m_geometry.size() != 0);
}
