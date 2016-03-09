#include "geoJsonSource.h"
#include "util/mapProjection.h"

#include "tileData.h"
#include "tile/tile.h"
#include "tile/tileID.h"
#include "tile/tileTask.h"
#include "util/geoJson.h"
#include "platform.h"
#include "log.h"

namespace Tangram {

GeoJsonSource::GeoJsonSource(const std::string& _name, const std::string& _urlTemplate,
                             int32_t _minDisplayZoom, int32_t _maxDisplayZoom, int32_t _maxZoom) :
    DataSource(_name, _urlTemplate, _minDisplayZoom, _maxDisplayZoom, _maxZoom) {
}

bool GeoJsonSource::process(const TileTask& _task, const MapProjection& _projection,
                            TileDataSink& _sink) const {

    auto& task = static_cast<const DownloadTileTask&>(_task);

    // Parse data into a JSON document
    const char* error;
    size_t offset;
    auto document = JsonParseBytes(task.rawTileData->data(),
                                   task.rawTileData->size(),
                                   &error, &offset);

    if (error) {
        LOGE("Json parsing failed on tile [%s]: %s (%u)",
             task.tileId().toString().c_str(), error, offset);
        return true;
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
        _sink.beginLayer("");
        GeoJson::processLayer(document, projFn, m_id, _sink);
    } else {
        for (auto layer = document.MemberBegin(); layer != document.MemberEnd(); ++layer) {
            if (GeoJson::isFeatureCollection(layer->value)) {
                if (_sink.beginLayer(layer->name.GetString())) {
                    GeoJson::processLayer(layer->value, projFn, m_id, _sink);
                }
            }
        }
    }

    // Discard original JSON object

    return true;
}

}
