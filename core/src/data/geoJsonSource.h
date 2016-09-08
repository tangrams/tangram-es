#pragma once

#include "dataSource.h"

namespace Tangram {

class GeoJsonSource: public DataSource {

protected:

    virtual std::shared_ptr<TileData> parse(const TileTask& _task,
                                            const MapProjection& _projection) const override;

public:

    GeoJsonSource(const std::string& _name, const std::string& _urlTemplate,
                  int32_t _minDisplayZoom, int32_t _maxDisplayZoom, int32_t _maxZoom);

};

}
