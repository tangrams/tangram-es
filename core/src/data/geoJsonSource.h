#pragma once

#include "dataSource.h"
#include "mapTile.h"
#include "tileData.h"


/* Extends NetworkDataSource class to read Mapzen's GeoJSON vector tiles */
class GeoJsonSource: public NetworkDataSource {
    
protected:
    
    virtual std::shared_ptr<TileData> parse(const MapTile& _tile, std::vector<char>& _rawData) override;
    
public:
    
    GeoJsonSource();
    
};
