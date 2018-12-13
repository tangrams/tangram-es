#include "tile/tileBuilder.h"

#include "data/properties.h"
#include "data/propertyItem.h"
#include "data/tileSource.h"
#include "gl/mesh.h"
#include "log.h"
#include "scene/dataLayer.h"
#include "scene/scene.h"
#include "selection/featureSelection.h"
#include "tile/tile.h"
#include "util/mapProjection.h"
#include "view/view.h"

namespace Tangram {

TileBuilder::TileBuilder(const Scene& _scene)
    : m_scene(_scene),
      m_styleContext(std::make_unique<StyleContext>()) {
}

TileBuilder::TileBuilder(const Scene& _scene, StyleContext* _styleContext)
    : m_scene(_scene),
      m_styleContext(std::unique_ptr<StyleContext>(_styleContext)) {
}

void TileBuilder::init() {
    m_styleContext->initFunctions(m_scene);

    // Initialize StyleBuilders
    for (const auto& style : m_scene.styles()) {
        if (auto builder = style->createBuilder()) {
            m_styleBuilder[style->getName()] = std::move(builder);
        }
    }
}

StyleBuilder* TileBuilder::getStyleBuilder(const std::string& _name) {
    auto it = m_styleBuilder.find(_name);
    if (it == m_styleBuilder.end()) { return nullptr; }

    return it->second.get();
}

void TileBuilder::applyStyling(const Feature& _feature, const SceneLayer& _layer) {

    // If no rules matched the feature, return immediately
    if (!m_ruleSet.match(_feature, _layer, *m_styleContext)) { return; }

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

        // Apply default draw rules defined for this style
        style->style().applyDefaultDrawRules(rule);

        if (!m_ruleSet.evaluateRuleForContext(rule, *m_styleContext)) {
            continue;
        }

        bool interactive = false;
        if (rule.get(StyleParamKey::interactive, interactive) && interactive) {
            if (selectionColor == 0) {
                selectionColor = m_scene.featureSelection()->nextColorIdentifier();
            }
            rule.selectionColor = selectionColor;
            rule.featureSelection = m_scene.featureSelection().get();
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

std::unique_ptr<Tile> TileBuilder::build(TileID _tileID, const TileData& _tileData, const TileSource& _source) {

    m_selectionFeatures.clear();

    auto tile = std::make_unique<Tile>(_tileID, _source.id(), _source.generation());

    tile->initGeometry(int(m_scene.styles().size()));

    m_styleContext->setKeywordZoom(_tileID.s);

    for (auto& builder : m_styleBuilder) {
        if (builder.second) { builder.second->setup(*tile); }
    }

    for (const auto& datalayer : m_scene.layers()) {

        if (datalayer.source() != _source.name()) { continue; }

        for (const auto& collection : _tileData.layers) {

            if (!collection.name.empty()) {
                const auto& dlc = datalayer.collections();
                bool layerContainsCollection =
                    std::find(dlc.begin(), dlc.end(), collection.name) != dlc.end();

                if (!layerContainsCollection) { continue; }
            }

            for (const auto& feat : collection.features) {
                applyStyling(feat, datalayer);
            }
        }
    }

    for (auto& builder : m_styleBuilder) {

        builder.second->addLayoutItems(m_labelLayout);
    }

    float tileSize = MapProjection::tileSize() * m_scene.pixelScale();

    m_labelLayout.process(_tileID, tile->getInverseScale(), tileSize);

    for (auto& builder : m_styleBuilder) {
        tile->setMesh(builder.second->style(), builder.second->build());
    }

    tile->setSelectionFeatures(m_selectionFeatures);

    return tile;
}

}
