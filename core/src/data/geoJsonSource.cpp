#include "geoJsonSource.h"
#include "util/mapProjection.h"

#include "tileData.h"
#include "tile/tile.h"
#include "tile/tileID.h"
#include "tile/tileTask.h"
#include "util/geoJson.h"
#include "platform.h"

#include "rapidjson/error/en.h"
#include "rapidjson/memorystream.h"
#include "rapidjson/encodings.h"
#include "rapidjson/encodedstream.h"

namespace Tangram {

GeoJsonSource::GeoJsonSource(const std::string& _name, const std::string& _urlTemplate, int32_t _maxZoom) :
    DataSource(_name, _urlTemplate, _maxZoom) {
}

std::shared_ptr<TileData> GeoJsonSource::parse(const TileTask& _task,
                                               const MapProjection& _projection) const {

    auto& task = static_cast<const DownloadTileTask&>(_task);

    std::shared_ptr<TileData> tileData = std::make_shared<TileData>();

    // parse written data into a JSON object
    rapidjson::Document doc;

    rapidjson::MemoryStream ms(task.rawTileData->data(), task.rawTileData->size());
    rapidjson::EncodedInputStream<rapidjson::UTF8<char>, rapidjson::MemoryStream> is(ms);

    doc.ParseStream(is);

    if (doc.HasParseError()) {

        size_t offset = doc.GetErrorOffset();
        const char* error = rapidjson::GetParseError_En(doc.GetParseError());
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
    // transform JSON data into a TileData using GeoJson functions
    for (auto layer = doc.MemberBegin(); layer != doc.MemberEnd(); ++layer) {
        tileData->layers.emplace_back(std::string(layer->name.GetString()));
        GeoJson::extractLayer(m_id, layer->value, tileData->layers.back(), projFn);
    }


    // Discard original JSON object and return TileData

    return tileData;

}

}
