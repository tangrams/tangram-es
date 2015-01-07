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

bool MapTile::addLabel(const Style& _style, std::unique_ptr<Label> _label) {

    m_labels[_style.getName()].push_back(std::move(_label));

    return true;
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

        float width = _view.getWidth();
        float height = _view.getHeight();

        float halfWidth = width * 0.5;
        float halfHeight = height * 0.5;

        glm::dmat4 mvp = _view.getViewProjectionMatrix() * m_modelMatrix;

        // lock the font context since the currently bound buffer has critical access
        ctx->m_contextMutex->lock();

        glfonsBindBuffer(ctx->m_fsContext, getTextBuffer(_style));

        for (auto& label : labels) {

            float alpha = label->m_alpha;

            glm::dvec4 position = glm::dvec4(label->m_worldPosition, 0.0, 1.0);

            // mimic gpu vertex projection to screen
            position = mvp * position;
            position = position / position.w; // perspective division

            // from normalized device coordinates to screen space coordinate system
            // top-left screen axis, y pointing down
            position.x = (position.x + 1) * halfWidth;
            position.y = (1 - position.y) * halfHeight;

            // don't display out of screen labels, and out of screen translations or not yet
            alpha = position.x > width || position.x < 0 ? 0.0 : alpha;
            alpha = position.y > height || position.y < 0 ? 0.0 : alpha;

            // cpu update of the transform texture, positionning the label in screen space
            glfonsTransform(ctx->m_fsContext, label->m_id, position.x, position.y, label->m_rotation, alpha);
        }

        // ask to push the transform texture to gpu
        // would be queued by the callback since we're not in main thread
        glfonsUpdateTransforms(ctx->m_fsContext, (void*) this);

        // unbind the buffer for context integrity
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

