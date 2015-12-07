#pragma once

#include "dataSource.h"

namespace Tangram {

class MVTSource : public DataSource {

protected:

    virtual std::shared_ptr<TileData> parse(const TileID& _tileId, const MapProjection& _projection,
                                            std::vector<char>& _rawData) const override;

public:

    MVTSource(const std::string& _name, const std::string& _urlTemplate);

};

}
