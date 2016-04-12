#pragma once

#include "dataSource.h"
#include "gl/texture.h"

namespace Tangram {

class RasterSource : public DataSource {

    TextureOptions m_texOptions;
    bool m_genMipmap;

protected:

    virtual std::shared_ptr<TileData> parse(const TileTask& _task,
                                            const MapProjection& _projection) const override;

public:

    RasterSource(const std::string& _name, const std::string& _urlTemplate, int32_t _maxZoom,
                 TextureOptions _options, bool genMipmap= false);

};

}
