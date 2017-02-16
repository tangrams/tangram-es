#pragma once

#include "data/tileSource.h"
#include "labels/labelCollider.h"
#include "scene/styleContext.h"
#include "scene/drawRule.h"

namespace Tangram {

class DataLayer;
class StyleBuilder;
class Tile;
class TileSource;
struct Feature;
struct Properties;
struct TileData;

class TileBuilder {

public:

    TileBuilder(std::shared_ptr<Scene> _scene);

    ~TileBuilder();

    StyleBuilder* getStyleBuilder(const std::string& _name);

    std::shared_ptr<Tile> build(TileID _tileID, const TileData& _data, const TileSource& _source);

    const Scene& scene() const { return *m_scene; }

private:

    // Determine and apply DrawRules for a @_feature
    void applyStyling(const Feature& _feature, const SceneLayer& _layer);

    std::shared_ptr<Scene> m_scene;

    StyleContext m_styleContext;
    DrawRuleMergeSet m_ruleSet;

    LabelCollider m_labelLayout;

    fastmap<std::string, std::unique_ptr<StyleBuilder>> m_styleBuilder;

    fastmap<uint32_t, std::shared_ptr<Properties>> m_selectionFeatures;
};

}
