#pragma once

#include "data/tileData.h"
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
class Tile;
class TileTask;
class StyleBuilder;

class TileBuilder : public TileDataSink {
public:

    TileBuilder(std::shared_ptr<Scene> _scene);

    ~TileBuilder();

    StyleBuilder* getStyleBuilder(const std::string& _name);

    // Process TileTask. On sucess _task.isReady() is true
    // and _task.tile() returns the created tile.
    std::shared_ptr<Tile> build(TileTask& _task);

    virtual bool beginLayer(const std::string& _layer) override;
    virtual bool matchFeature(const Feature& _feature) override;
    virtual void addFeature(const Feature& _feature) override;

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

    std::string m_activeSource;
    std::vector<const DataLayer*> m_activeLayers;
    const DataLayer* m_matchedLayer = nullptr;

    std::shared_ptr<Tile> m_tile;
};

}
