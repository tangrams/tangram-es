#pragma once

#include "dataSource.h"
#include "tile/tile.h"
#include "tileData.h"

namespace Tangram {

class GeoJsonSource: public DataSource {

protected:

    virtual std::shared_ptr<TileData> parse(const Tile& _tile, std::vector<char>& _rawData) const override;

public:

    GeoJsonSource(const std::string& _name, const std::string& _urlTemplate);

};

}
