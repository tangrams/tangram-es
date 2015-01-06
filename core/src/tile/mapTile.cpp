#include "mapTile.h"
#include "style/style.h"
#include "util/tileID.h"

#define GLM_FORCE_RADIANS
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

MapTile::MapTile(TileID _id, const MapProjection& _projection) : m_id(_id),  m_projection(&_projection) {

    glm::dvec4 bounds = _projection.TileBounds(_id); // [x: xmin, y: ymin, z: xmax, w: ymax]
    
    m_scale = 0.5 * glm::abs(bounds.x - bounds.z);
    m_inverseScale = 1.0/m_scale;
    
    // negative y coordinate: to change from y down to y up (tile system has y down and gl context we use has y up).
    m_tileOrigin = glm::dvec2(0.5*(bounds.x + bounds.z), -0.5*(bounds.y + bounds.w));
    
    m_modelMatrix = glm::dmat4(1.0);
    
    // Translate model matrix to origin of tile
    m_modelMatrix = glm::translate(m_modelMatrix, glm::dvec3(m_tileOrigin.x, m_tileOrigin.y, 0.0));
    
    // Scale model matrix to size of tile
    m_modelMatrix = glm::scale(m_modelMatrix, glm::dvec3(m_scale));

}

MapTile::~MapTile() {

    m_geometry.clear();

}

void MapTile::addGeometry(const Style& _style, std::unique_ptr<VboMesh> _mesh) {

    m_geometry[_style.getName()] = std::move(_mesh); // Move-construct a unique_ptr at the value associated with the given style

}

void MapTile::addLabel(const Style& _style, std::unique_ptr<Label> _label) {

    m_labels[_style.getName()].push_back(std::move(_label));

}

void MapTile::setTextBuffer(const Style& _style, fsuint _textBuffer) {

    m_textBuffer[_style.getName()] = _textBuffer;

}

fsuint MapTile::getTextBuffer(const Style& _style) const {

    auto it = m_textBuffer.find(_style.getName());

    if (it != m_textBuffer.end()) {
        return it->second;
    }

    return 0;
}

void MapTile::update(float _dt, const Style& _style, View& _view) {

    auto& labels = m_labels[_style.getName()];

    // update label positions
    if (labels.size() > 0) {

        std::shared_ptr<FontContext> ctx = labels[0]->m_fontContext;

        ctx->m_contextMutex->lock();

        glfonsBindBuffer(ctx->m_fsContext, getTextBuffer(_style));

        for (auto& label : labels) {

            float alpha = label->m_alpha;

            glm::dvec4 position = glm::dvec4(label->m_worldPosition, 0.0, 1.0);

            // project to screen and perform perspective division
            position = _view.getViewProjectionMatrix() * m_modelMatrix * position;
            position = position / position.w;

            // from normalized device coordinates to screen space coordinate system
            position.x = (position.x * _view.getWidth() * 0.5) + _view.getWidth() * 0.5;
            position.y = -(position.y * _view.getHeight() * 0.5) + _view.getHeight() * 0.5;

            alpha = position.x > _view.getWidth() || position.x < 0 ? 0.0 : alpha;
            alpha = position.y > _view.getHeight() || position.y < 0 ? 0.0 : alpha;

            glfonsTransform(ctx->m_fsContext, label->m_id, position.x, position.y, label->m_rotation, alpha);
        }

        glfonsUpdateTransforms(ctx->m_fsContext, (void*) this);
        glfonsBindBuffer(ctx->m_fsContext, 0);

        ctx->m_contextMutex->unlock();
    }
    
}

void MapTile::draw(const Style& _style, const glm::dmat4& _viewProjMatrix) {

    const std::unique_ptr<VboMesh>& styleMesh = m_geometry[_style.getName()];

    if (styleMesh) {

        std::shared_ptr<ShaderProgram> shader = _style.getShaderProgram();

        glm::dmat4 modelViewProjMatrix = _viewProjMatrix * m_modelMatrix;

        // NOTE : casting to float, but loop over the matrix values  
        double* first = &modelViewProjMatrix[0][0];
        std::vector<float> fmvp(first, first + 16);

        shader->setUniformMatrix4f("u_modelViewProj", &fmvp[0]);

        styleMesh->draw(shader);
    }
}

