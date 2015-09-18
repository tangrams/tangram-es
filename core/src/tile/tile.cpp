#include "tile.h"

#include "data/dataSource.h"
#include "scene/scene.h"
#include "scene/dataLayer.h"
#include "style/style.h"
#include "view/view.h"
#include "tile/tileID.h"
#include "gl/vboMesh.h"
#include "gl/shaderProgram.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

namespace Tangram {

Tile::Tile(TileID _id, const MapProjection& _projection) :
    m_id(_id),
    m_projection(&_projection),
    m_visible(false),
    m_priority(0) {

    BoundingBox bounds(_projection.TileBounds(_id));

    m_scale = 0.5 * bounds.width();
    m_inverseScale = 1.0/m_scale;

    m_tileOrigin = bounds.center();
    // negative y coordinate: to change from y down to y up (tile system has y down and gl context we use has y up).
    m_tileOrigin.y *= -1.0;

    // Init model matrix to size of tile
    m_modelMatrix = glm::scale(glm::mat4(1.0), glm::vec3(m_scale));
}

Tile::~Tile() {

}

void Tile::build(StyleContext& _ctx, const Scene& _scene, const TileData& _data, const DataSource& _source) {

    const auto& layers = _scene.layers();

    _ctx.setGlobalZoom(m_id.z);

    for (auto& style : _scene.styles()) {
        style->onBeginBuildTile(*this);
    }

    for (const auto& datalayer : layers) {

        if (datalayer.source() != _source.name()) { continue; }

        for (const auto& collection : _data.layers) {

            if (!collection.name.empty() && collection.name != datalayer.collection()) { continue; }

            for (const auto& feat : collection.features) {
                _ctx.setFeature(feat);

                std::vector<DrawRule> rules;
                datalayer.match(feat, _ctx, rules);

                for (auto& rule : rules) {
                    auto* style = _scene.findStyle(rule.style);

                    if (style) {
                        rule.eval(_ctx);
                        style->buildFeature(*this, feat, rule);
                    }
                }
            }
        }
    }

    for (auto& style : _scene.styles()) {
        style->onEndBuildTile(*this);
    }

    for (auto& geometry : m_geometry) {
        geometry.second->compileVertexBuffer();
    }
}

void Tile::update(float _dt, const View& _view) {

    // Apply tile-view translation to the model matrix
    const auto& viewOrigin = _view.getPosition();
    m_modelMatrix[3][0] = m_tileOrigin.x - viewOrigin.x;
    m_modelMatrix[3][1] = m_tileOrigin.y - viewOrigin.y;

}

void Tile::draw(const Style& _style, const View& _view) {

    const auto& styleMesh = m_geometry[_style.getName()];

    if (styleMesh) {

        auto& shader = _style.getShaderProgram();

        glm::mat4 modelViewMatrix = _view.getViewMatrix() * m_modelMatrix;
        glm::mat4 modelViewProjMatrix = _view.getViewProjectionMatrix() * m_modelMatrix;

        shader->setUniformMatrix4f("u_modelView", glm::value_ptr(modelViewMatrix));
        shader->setUniformMatrix4f("u_modelViewProj", glm::value_ptr(modelViewProjMatrix));
        shader->setUniformMatrix3f("u_normalMatrix", glm::value_ptr(_view.getNormalMatrix()));

        // Set the tile zoom level, using the sign to indicate whether the tile is a proxy
        shader->setUniformf("u_tile_zoom", m_proxyCounter > 0 ? -m_id.z : m_id.z);

        styleMesh->draw(*shader);
    }
}

std::unique_ptr<VboMesh>& Tile::getMesh(const Style& _style) {
    return m_geometry[_style.getName()];
}

size_t Tile::getMemoryUsage() const {
    if (m_memoryUsage == 0) {
        for (auto& entry : m_geometry) {
            if (entry.second) {
                m_memoryUsage += entry.second->bufferSize();
            }
        }
    }

    return m_memoryUsage;
}

}
