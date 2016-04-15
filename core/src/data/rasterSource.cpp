#include "rasterSource.h"
#include "propertyItem.h"
#include "util/mapProjection.h"

#include "tileData.h"
#include "tile/tile.h"
#include "tile/tileTask.h"
#include "util/geoJson.h"
#include "platform.h"

namespace Tangram {


RasterSource::RasterSource(const std::string& _name, const std::string& _urlTemplate, int32_t _maxZoom,
                            TextureOptions _options, bool _genMipmap) :
        DataSource(_name, _urlTemplate, _maxZoom), m_texOptions(_options),m_genMipmap(_genMipmap) {
}

std::shared_ptr<TileData> RasterSource::parse(const TileTask& _task, const MapProjection& _projection) const {

    std::shared_ptr<TileData> tileData = std::make_shared<TileData>();

    Feature rasterFeature;
    rasterFeature.geometryType = GeometryType::polygons;
    rasterFeature.polygons = { { {
                                         {0.0f, 0.0f, 0.0f},
                                         {1.0f, 0.0f, 0.0f},
                                         {1.0f, 1.0f, 0.0f},
                                         {0.0f, 1.0f, 0.0f},
                                         {0.0f, 0.0f, 0.0f}
                                 } } };
    rasterFeature.props = Properties();

    tileData->layers.emplace_back("");
    tileData->layers.back().features.push_back(rasterFeature);
    return tileData;

}

std::shared_ptr<Texture> RasterSource::texture(const TileTask& _task) {

    auto tileID = _task.tileId();
    if (m_textures.find(tileID) != m_textures.end()) { return m_textures.at(tileID); }

    std::lock_guard<std::mutex> lock(m_textureMutex);
    auto &task = static_cast<const DownloadTileTask &>(_task);
    auto udata = (unsigned char*)task.rawTileData->data();
    std::shared_ptr<Texture> texture(new Texture(udata, task.rawTileData->size(), m_texOptions, m_genMipmap, true));

    if (texture->hasValidData()) {
        m_textures[tileID] = texture;
        return texture;
    } else {
        LOGW("Texture for data source %s has failed to decode", m_name.c_str());
        return nullptr;
    }
}

void RasterSource::clearTextures() {
    for (auto& raster: m_rasters) {
        raster->clearTextures();
    }
    m_textures.clear();
}

void RasterSource::clearTexture(const TileID &id) {
    for (auto& raster: m_rasters) {
        raster->clearTexture(id);
    }
    m_textures.erase(id);
}

}
