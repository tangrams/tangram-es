#pragma once

#include "dataSource.h"

namespace Tangram {

class TopoJsonSource : public DataSource {

protected:

    virtual std::shared_ptr<TileData> parse(const TileTask& _task,
                                            const MapProjection& _projection) const override;

public:

    TopoJsonSource(const std::string& _name, const std::string& _urlTemplate, const std::string& _mbtiles,
                   int32_t _minDisplayZoom, int32_t _maxDisplayZoom, int32_t _maxZoom);

    // TODO: We need to register this MIME Media Type with the IANA
    virtual const char* mimeType() override { return "application/topo+json"; };
};

}
