#include <iostream>
#include <memory>
#include <string>
#include <sstream>

//#include "sceneDirector/sceneDirector.h"
#include "tileManager/tileManager.h"
#include "dataSource/dataSource.h"
#include "mapTile/mapTile.h"

int main()
{
    // new DataSource created
    std::unique_ptr<DataSource> ds_ptr(new MapzenVectorTileJson);

    // Singleton instance of TileManager
    TileManager tileManagerInst = TileManager::GetInstance();
    // Moved dataSource to be "owned" by tileManager
    tileManagerInst.AddDataSource(std::move(ds_ptr));
    
    bool newTiles = tileManagerInst.CheckNewTileStatus();
    std::vector<MapTile*> visibleTiles = tileManagerInst.GetVisibleTiles();
    for(auto tile : visibleTiles) {
        std::ostringstream tileID (std::ostringstream::ate);
        tileID << tile->m_MercXYZ.x << "_" << tile->m_MercXYZ.y << "_" << tile->m_MercXYZ.z;
        // tileManager gets the dataSoure (only one), which gets the data stored for every tile it has.
        std::cout<<*((tileManagerInst.GetDataSources().at(0)->GetData(tileID.str()).get()));
    }

    return 0;
}
