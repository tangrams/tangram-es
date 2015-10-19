#include "tile.h"

#include "data/dataSource.h"
#include "scene/scene.h"
#include "scene/dataLayer.h"
#include "scene/styleContext.h"
#include "style/material.h"
#include "style/style.h"
#include "view/view.h"
#include "tile/tileID.h"
#include "labels/labelMesh.h"
#include "gl/vboMesh.h"
#include "gl/shaderProgram.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <algorithm>

namespace Tangram {

Tile::Tile(TileID _id, const MapProjection& _projection) :
    m_id(_id),
    m_projection(&_projection),
    m_visible(false),
    m_priority(0) {

    BoundingBox bounds(_projection.TileBounds(_id));

    m_scale = bounds.width();
    m_inverseScale = 1.0/m_scale;

    updateTileOrigin();

    // Init model matrix to size of tile
    m_modelMatrix = glm::scale(glm::mat4(1.0), glm::vec3(m_scale));
}

Tile::~Tile() {

}

void Tile::updateTileOrigin() {
    BoundingBox bounds(m_projection->TileBounds(m_id));

    m_tileOrigin = { bounds.min.x, bounds.max.y }; // South-West corner
    // negative y coordinate: to change from y down to y up (tile system has y down and gl context we use has y up).
    m_tileOrigin.y *= -1.0;

    auto mapBound = m_projection->MapBounds();
    auto mapSpan = mapBound.max.x - mapBound.min.x;
    switch (m_id.wrap) {
        case TileWrap::none:
            break;
        case TileWrap::positive:
            m_tileOrigin.x += mapSpan;
            break;
        case TileWrap::negative:
            m_tileOrigin.x -= mapSpan;
            break;
    }
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

            const auto& dlc = datalayer.collections();
            bool layerContainsCollection = std::find(dlc.begin(), dlc.end(), collection.name) != dlc.end();

            if (!collection.name.empty() && !layerContainsCollection) { continue; }

            for (const auto& feat : collection.features) {
                _ctx.setFeature(feat);

                std::vector<DrawRule> rules;
                datalayer.match(feat, _ctx, rules);

                for (auto& rule : rules) {
                    auto* style = _scene.findStyle(rule.getStyleName());

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

void Tile::reset() {
    for (auto& entry : m_geometry) {
        if (!entry.second) { continue; }
        auto labelMesh = dynamic_cast<LabelMesh*>(entry.second.get());
        if (!labelMesh) { continue; }
        labelMesh->reset();
    }
}

void Tile::draw(const Style& _style, const View& _view) {

    const auto& styleMesh = m_geometry[_style.getName()];

    if (styleMesh) {

        auto& shader = _style.getShaderProgram();

        glm::mat4 modelViewProjMatrix = _view.getViewProjectionMatrix() * m_modelMatrix;
        float zoomAndProxy = m_proxyCounter > 0 ? -m_id.z : m_id.z;

        shader->setUniformMatrix4f("u_model", glm::value_ptr(m_modelMatrix));
        shader->setUniformMatrix4f("u_modelViewProj", glm::value_ptr(modelViewProjMatrix));
        shader->setUniformf("u_tile_origin", m_tileOrigin.x, m_tileOrigin.y, zoomAndProxy);

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
