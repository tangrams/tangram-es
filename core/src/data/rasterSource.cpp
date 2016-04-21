#include "rasterSource.h"

#include "tileData.h"
#include "tile/tile.h"
#include "tile/tileTask.h"
#include "util/pbfParser.h"
#include "platform.h"

namespace Tangram {


RasterSource::RasterSource(const std::string& _name, const std::string& _urlTemplate, int32_t _maxZoom,
                            TextureOptions _options, bool _genMipmap) :
    m_texOptions(_options), m_genMipmap(_genMipmap), DataSource(_name, _urlTemplate, _maxZoom) {
}

std::shared_ptr<TileData> RasterSource::parse(const TileTask& _task, const MapProjection& _projection) const {

    //TODO: Return TileData with a featureCollection having a quad of TileScale Dimentions.
    /*auto& task = static_cast<const DownloadTileTask&>(_task);

    std::shared_ptr<TileData> tileData = std::make_shared<TileData>();

    // Parse data into a JSON document
    const char* error;
    size_t offset;
    auto document = JsonParseBytes(task.rawTileData->data(), task.rawTileData->size(), &error, &offset);

    if (error) {
        LOGE("Json parsing failed on tile [%s]: %s (%u)", task.tileId().toString().c_str(), error, offset);
        return tileData;
    }

    BoundingBox tileBounds(_projection.TileBounds(task.tileId()));
    glm::dvec2 tileOrigin = {tileBounds.min.x, tileBounds.max.y*-1.0};
    double tileInverseScale = 1.0 / tileBounds.width();

    const auto projFn = [&](glm::dvec2 _lonLat){
        glm::dvec2 tmp = _projection.LonLatToMeters(_lonLat);
        return Point {
                (tmp.x - tileOrigin.x) * tileInverseScale,
                (tmp.y - tileOrigin.y) * tileInverseScale,
                0
        };
    };

    // Transform JSON data into TileData using GeoJson functions
    if (GeoJson::isFeatureCollection(document)) {
        tileData->layers.push_back(GeoJson::getLayer(document, projFn, m_id));
    } else {
        for (auto layer = document.MemberBegin(); layer != document.MemberEnd(); ++layer) {
            if (GeoJson::isFeatureCollection(layer->value)) {
                tileData->layers.push_back(GeoJson::getLayer(layer->value, projFn, m_id));
                tileData->layers.back().name = layer->name.GetString();
            }
        }
    }


    // Discard original JSON object and return TileData

    return tileData;*/
    return nullptr;

}

}
