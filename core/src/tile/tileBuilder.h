#pragma once

#include "data/dataSource.h"
#include "scene/styleContext.h"
#include "scene/drawRule.h"

namespace Tangram {

class DataLayer;
class DataSource;
class Tile;
struct TileData;
class StyleBuilder;

class TileBuilder : public TileDataSink {
public:

    TileBuilder();

    ~TileBuilder();

    StyleBuilder* getStyleBuilder(const std::string& _name);

    void setScene(std::shared_ptr<Scene> _scene);

    void begin(const TileID& _tileID, const DataSource& _source);

    std::shared_ptr<Tile> build();

    virtual bool beginLayer(const std::string& _layer) override;
    virtual bool matchFeature(const Feature& _feature) override;
    virtual void addFeature(const Feature& _feature) override;

    const Scene& scene() const { return *m_scene; }

    Tile& tile() const { return *m_tile; }

private:
    std::shared_ptr<Scene> m_scene;

    StyleContext m_styleContext;
    DrawRuleMergeSet m_ruleSet;

    fastmap<std::string, std::unique_ptr<StyleBuilder>> m_styleBuilder;

    std::vector<const DataLayer*> m_activeLayers;

    std::shared_ptr<Tile> m_tile;
};

}
