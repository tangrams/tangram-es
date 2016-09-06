#pragma once

#include "dataSource.h"

namespace Tangram {

class TopoJsonSource : public DataSource {

protected:

    virtual std::shared_ptr<TileData> parse(const TileTask& _task,
                                            const MapProjection& _projection) const override;

public:

    TopoJsonSource(const std::string& _name, const std::string& _urlTemplate, int32_t minDisplayZoom, int32_t maxZoom);

};

}
