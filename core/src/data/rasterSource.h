#pragma once


#include <tile/tileTask.h>
#include "tile/tileHash.h"
#include "dataSource.h"
#include "gl/texture.h"

#include <functional>
#include <unordered_map>
#include <mutex>

namespace Tangram {

class RasterSource : public DataSource {

    TextureOptions m_texOptions;
    bool m_genMipmap;
    std::unordered_map<TileID, std::shared_ptr<Texture>> m_textures;
    std::mutex m_textureMutex;

protected:

    virtual std::shared_ptr<TileData> parse(const TileTask& _task,
                                            const MapProjection& _projection) const override;

public:

    RasterSource(const std::string& _name, const std::string& _urlTemplate, int32_t _maxZoom,
                 TextureOptions _options, bool genMipmap= false);

    virtual std::shared_ptr<Texture> texture(const TileTask& _task) override;
    virtual void clearTextures() override;
    virtual void clearTexture(const TileID& id) override;

};

}
