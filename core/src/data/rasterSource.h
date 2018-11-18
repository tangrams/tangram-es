#pragma once

#include "data/tileSource.h"
#include "gl/texture.h"
#include "tile/tileTask.h"
#include "tile/tileHash.h"

#include <functional>
#include <unordered_map>
#include <mutex>

namespace Tangram {

class RasterTileTask;

class RasterSource : public TileSource {

    TextureOptions m_texOptions;
    std::unordered_map<TileID, std::shared_ptr<Texture>> m_textures;

    std::shared_ptr<Texture> m_emptyTexture;

protected:

    std::shared_ptr<TileData> parse(const TileTask& _task) const override;

public:

    RasterSource(const std::string& _name, std::unique_ptr<DataSource> _sources,
                 TextureOptions _options, TileSource::ZoomOptions _zoomOptions = {});

    // TODO Is this always PNG or can it also be JPEG?
    virtual const char* mimeType() const override { return "image/png"; };

    virtual std::shared_ptr<TileTask> createTask(Scene& _scene, TileID _tile, int _subTask) override;

    virtual void clearRasters() override;
    virtual void clearRaster(const TileID& id) override;
    virtual bool isRaster() const override { return true; }

    std::shared_ptr<Texture> createTexture(const std::vector<char>& _rawTileData);

    Raster getRaster(const TileTask& _task);

};

}
