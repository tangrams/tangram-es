#include "rasterSource.h"
#include "propertyItem.h"
#include "util/mapProjection.h"

#include "tileData.h"
#include "tile/tile.h"
#include "tile/tileTask.h"
#include "util/geoJson.h"

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

std::unique_ptr<Texture> RasterSource::getTexture(const TileTask& _task) const {

    auto &task = static_cast<const DownloadTileTask &>(_task);
    auto udata = (unsigned char*)task.rawTileData->data();
    std::unique_ptr<Texture> texture(new Texture(udata, task.rawTileData->size(), m_texOptions, m_genMipmap));
    return std::move(texture);

}

}
