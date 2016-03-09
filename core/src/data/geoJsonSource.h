#pragma once

#include "data/tileSource.h"

namespace Tangram {

class GeoJsonSource: public TileSource {

public:
    using TileSource::TileSource;

protected:

    virtual bool process(const TileTask& _task,
                         const MapProjection& _projection,
                         TileDataSink& _sink) const override;

    // http://www.iana.org/assignments/media-types/application/geo+json
    virtual const char* mimeType() override { return "application/geo+json"; };

};

}
