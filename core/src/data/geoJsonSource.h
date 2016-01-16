#pragma once

#include "dataSource.h"

namespace Tangram {

class GeoJsonSource: public DataSource {

protected:

    virtual bool process(const TileTask& _task,
                         const MapProjection& _projection,
                         TileDataSink& _sink) const override;

public:

    GeoJsonSource(const std::string& _name, const std::string& _urlTemplate, int32_t _maxZoom);

};

}
