#pragma once

#include <map>
#include <vector>
#include <memory>

#include "glm/fwd.hpp"
#include "glm/glm.hpp"

#include "util/tileID.h"

class SceneDefinition;
class MapTile;
class ViewModule;
class DataSource;

/* Singleton container of <MapTile>s
 *
 * TileManager is a singleton that maintains a set of MapTiles based on the current view into the map
 */
class TileManager {

public:
    
    /* Returns the single instance of the TileManager */
    static std::unique_ptr<TileManager> GetInstance() {
        static std::unique_ptr<TileManager> instance (new TileManager());
        return std::move(instance);
    }

    /* Constructs a TileManager using move semantics */
    TileManager(TileManager&& _other);

    virtual ~TileManager();

    /* Sets the view for which the TileManager will maintain tiles */
    void setView(std::shared_ptr<ViewModule> _view) { m_viewModule = _view; }

    /* Sets the scene defintion which the TileManager will use to style tiles */
    void setSceneDefinition(std::shared_ptr<SceneDefinition> _sceneDef) { m_sceneDefinition = _sceneDef; }

    /* Adds a <DataSource> from which tile data should be retrieved */
    void addDataSource(std::shared_ptr<DataSource> _source) { m_dataSources.push_back(_source); }

    /* Updates visible tile set if necessary
     * 
     * Contacts the <ViewModule> to determine whether the set of visible tiles has changed; if so,
     * constructs or disposes tiles as needed and returns true
     */
    bool updateTileSet();

    /* Returns the set of currently visible tiles */
    const std::map<TileID, std::unique_ptr<MapTile>>& getVisibleTiles() { return m_tileSet; }

private:

    TileManager();

    std::shared_ptr<ViewModule> m_viewModule;
    std::shared_ptr<SceneDefinition> m_sceneDefinition;

    std::map<TileID, std::unique_ptr<MapTile>> m_tileSet;

    std::vector<std::shared_ptr<DataSource>> m_dataSources;

    std::vector<TileID> m_tilesToAdd;

    void addTile(const TileID& _tileID);
    void removeTile(const std::map<TileID, std::unique_ptr<MapTile>>::iterator& _tileIter);

};
