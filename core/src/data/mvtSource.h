#pragma once

#include "dataSource.h"

namespace Tangram {

class MVTSource : public DataSource {

protected:

    virtual std::shared_ptr<TileData> parse(const TileTask& _task,
                                            const MapProjection& _projection) const override;

public:

    MVTSource(const std::string& _name, const std::string& _urlTemplate, const std::string& _mbtiles,
              int32_t _minDisplayZoom, int32_t _maxDisplayZoom, int32_t _maxZoom);

    // http://www.iana.org/assignments/media-types/application/vnd.mapbox-vector-tile
    virtual const char* mimeType() override { return "application/vnd.mapbox-vector-tile"; };
};

}
