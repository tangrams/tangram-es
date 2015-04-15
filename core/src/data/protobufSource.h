#pragma once

#include "dataSource.h"
#include "mapTile.h"
#include "tileData.h"


class ProtobufSource : public NetworkDataSource {
    
protected:
    
    virtual std::shared_ptr<TileData> parse(const MapTile& _tile, const char* _rawData, const int _dataSize) override;
    
public:
    
    ProtobufSource();
    
};
