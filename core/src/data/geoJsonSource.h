#pragma once

#include "data/tileSource.h"

namespace Tangram {

class GeoJsonSource: public TileSource {

public:
    using TileSource::TileSource;

protected:

    virtual std::shared_ptr<TileData> parse(const TileTask& _task,
                                            const MapProjection& _projection) const override;

    // http://www.iana.org/assignments/media-types/application/geo+json
    virtual const char* mimeType() override { return "application/geo+json"; };

};

}
