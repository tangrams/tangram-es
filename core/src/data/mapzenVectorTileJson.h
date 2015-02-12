#pragma once

#include "dataSource.h"
#include "mapTile.h"
#include "tileData.h"


/* Extends NetworkDataSource class to read Mapzen's GeoJSON vector tiles */
class MapzenVectorTileJson: public NetworkDataSource {
    
protected:
    
    virtual std::shared_ptr<TileData> parse(const MapTile& _tile, std::stringstream& _in) override;
    
public:
    
    MapzenVectorTileJson();
    
};
