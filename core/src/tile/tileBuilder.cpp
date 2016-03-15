#include "tile/tileBuilder.h"

#include "data/properties.h"
#include "data/propertyItem.h"
#include "data/tileSource.h"
#include "gl/mesh.h"
#include "log.h"
#include "scene/dataLayer.h"
#include "scene/scene.h"
#include "selection/featureSelection.h"
#include "style/style.h"
#include "tile/tile.h"
#include "util/mapProjection.h"
#include "view/view.h"
#include "tile/tileTask.h"

namespace Tangram {

TileBuilder::TileBuilder(std::shared_ptr<Scene> _scene)
    : m_scene(_scene) {

    m_styleContext.initFunctions(*_scene);

    // Initialize StyleBuilders
    for (auto& style : _scene->styles()) {
        m_styleBuilder[style->getName()] = style->createBuilder();
    }
}

TileBuilder::~TileBuilder() {}

StyleBuilder* TileBuilder::getStyleBuilder(const std::string& _name) {
    auto it = m_styleBuilder.find(_name);
    if (it == m_styleBuilder.end()) { return nullptr; }

    return it->second.get();
}

void TileBuilder::applyStyling(const Feature& _feature, const SceneLayer& _layer) {

    uint32_t selectionColor = 0;
    bool added = false;

    // For each matched rule, find the style to be used and
    // build the feature with the rule's parameters
    for (auto& rule : m_ruleSet.matchedRules()) {

        StyleBuilder* style = getStyleBuilder(rule.getStyleName());

        if (!style) {
            LOGN("Invalid style %s", rule.getStyleName().c_str());
            continue;
        }

        if (!m_ruleSet.evaluateRuleForContext(rule, m_styleContext)) {
            continue;
        }

        bool interactive = false;
        if (rule.get(StyleParamKey::interactive, interactive) && interactive) {
            if (selectionColor == 0) {
                selectionColor = m_scene->featureSelection()->nextColorIdentifier();
            }
            rule.selectionColor = selectionColor;
            rule.featureSelection = m_scene->featureSelection().get();
        } else {
            rule.selectionColor = 0;
        }

        // build outline explicitly with outline style
        const auto& outlineStyleName = rule.findParameter(StyleParamKey::outline_style);
        if (outlineStyleName) {
            auto& styleName = outlineStyleName.value.get<std::string>();
            auto* outlineStyle = getStyleBuilder(styleName);
            if (!outlineStyle) {
                LOGN("Invalid style %s", styleName.c_str());
            } else {
                rule.isOutlineOnly = true;
                outlineStyle->addFeature(_feature, rule);
                rule.isOutlineOnly = false;
            }
        }

        // build feature with style
        added |= style->addFeature(_feature, rule);
    }

    if (added && (selectionColor != 0)) {
        m_selectionFeatures[selectionColor] = std::make_shared<Properties>(_feature.props);
    }
}

bool TileBuilder::beginLayer(const std::string& _layerName) {

    m_activeLayers.clear();

    for (const auto& layer : m_scene->layers()) {

        if (layer.source() != m_activeSource) {
            continue;
        }

        if (!_layerName.empty()) {
            const auto& dlc = layer.collections();
            if (std::find(dlc.begin(), dlc.end(), _layerName) == dlc.end()) {
                continue;
            }
        }
        m_activeLayers.push_back(&layer);
    }

    return !m_activeLayers.empty();
}

// TileDataSink callback
bool TileBuilder::matchFeature(const Feature& _feature) {
    m_matchedLayer = nullptr;

    for (auto* layer : m_activeLayers) {
        if(m_ruleSet.match(_feature, *layer, m_styleContext)) {
            // keep reference to matched layer for addFeature
            m_matchedLayer = layer;
            return true;
        }
    }

    return false;
}

// TileDataSink callback
void TileBuilder::addFeature(const Feature& _feature) {

    // Require that matchFeature found a layer.
    if (!m_matchedLayer) { return; }

    for (const auto* layer : m_activeLayers) {

        if (m_matchedLayer) {
            // Skip until first matched layer
            if (m_matchedLayer != layer) {
                continue;
            }
            m_matchedLayer = nullptr;

        } else if (!m_ruleSet.match(_feature, *layer, m_styleContext)) {
            continue;
        }

        applyStyling(_feature, *layer);
    }
}

std::shared_ptr<Tile> TileBuilder::build(TileTask& _task) {

    m_selectionFeatures.clear();

    auto tile = std::make_shared<Tile>(_task.tileId(),
                                       *m_scene->mapProjection(),
                                       &_task.source());

    tile->initGeometry(m_scene->styles().size());

    m_styleContext.setKeywordZoom(_task.tileId().s);

    for (auto& builder : m_styleBuilder) {
        if (builder.second) {
            builder.second->setup(*tile);
        }
    }

    m_activeSource = _task.source().name();

    // Pass 'this' as TileDataSink
    if (!_task.source().process(_task, *m_scene->mapProjection(), *this)) {
        _task.cancel();
        return nullptr;
    }

    for (auto& builder : m_styleBuilder) {

        builder.second->addLayoutItems(m_labelLayout);
    }

    float tileSize = m_scene->mapProjection()->TileSize() * m_scene->pixelScale();

    m_labelLayout.process(_task.tileId(), tile->getInverseScale(), tileSize);

    for (auto& builder : m_styleBuilder) {
        tile->setMesh(builder.second->style(), builder.second->build());
    }

    tile->setSelectionFeatures(m_selectionFeatures);

    return tile;
}

}
