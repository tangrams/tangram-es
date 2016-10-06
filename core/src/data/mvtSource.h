#pragma once

#include "dataSource.h"

namespace Tangram {

class MVTSource : public DataSource {

public:
    using DataSource::DataSource;

protected:

    virtual std::shared_ptr<TileData> parse(const TileTask& _task,
                                            const MapProjection& _projection) const override;

};

}
