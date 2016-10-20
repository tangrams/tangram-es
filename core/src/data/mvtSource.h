#pragma once

#include "tileSource.h"

namespace Tangram {

class MVTSource : public TileSource {

public:
    using TileSource::TileSource;

protected:

    virtual std::shared_ptr<TileData> parse(const TileTask& _task,
                                            const MapProjection& _projection) const override;

};

}
