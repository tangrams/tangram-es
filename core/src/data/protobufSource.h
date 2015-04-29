#pragma once

#include "dataSource.h"
#include "mapTile.h"
#include "tileData.h"


class ProtobufSource : public DataSource {

protected:

    virtual std::shared_ptr<TileData> parse(const MapTile& _tile, std::stringstream& _in) override;

public:

    ProtobufSource();

};
