#pragma once

#include "data/tileSource.h"
#include "gl/texture.h"
#include "tile/tileTask.h"
#include "tile/tileHash.h"

#include <functional>
#include <map>
#include <mutex>

namespace Tangram {

class RasterTileTask;

class RasterSource : public TileSource {

    using Cache = std::map<TileID, std::shared_ptr<Texture>>;
    Cache m_textures;

    TextureOptions m_texOptions;

    std::shared_ptr<Texture> m_emptyTexture;

    friend class RasterTileTask;
    friend class TileSource;
protected:
    std::shared_ptr<TileData> m_tileData;

    std::shared_ptr<TileData> parse(const TileTask& _task) const override;

    std::shared_ptr<RasterTileTask> createRasterTask(TileID _tileId, bool subTask);

    void addRasterTask(TileTask& _tileTask);

    std::shared_ptr<Texture> createTexture(TileID _tile, const std::vector<char>& _rawTileData);

    Raster addRaster(const TileID& _tileId, std::shared_ptr<Texture> _texture);

public:

    RasterSource(const std::string& _name, std::unique_ptr<DataSource> _sources,
                 TextureOptions _options, TileSource::ZoomOptions _zoomOptions = {});

    // TODO Is this always PNG or can it also be JPEG?
    virtual const char* mimeType() const override { return "image/png"; };

    void loadTileData(std::shared_ptr<TileTask> _task, TileTaskCb _cb) override;

    virtual std::shared_ptr<TileTask> createTask(TileID _tile) override;

    virtual bool isRaster() const override { return true; }

};

}
