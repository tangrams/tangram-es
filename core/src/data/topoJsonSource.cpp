#include "topoJsonSource.h"

#include "tileData.h"
#include "tile/tile.h"
#include "tile/tileTask.h"
#include "util/mapProjection.h"
#include "util/topoJson.h"
#include "platform.h"


namespace Tangram {

TopoJsonSource::TopoJsonSource(const std::string& _name, const std::string& _urlTemplate,
                               int32_t minDisplayZoom, int32_t _maxDisplayZoom, int32_t _maxZoom) :
    DataSource(_name, _urlTemplate, minDisplayZoom, _maxDisplayZoom, _maxZoom) {
}

std::shared_ptr<TileData> TopoJsonSource::parse(const TileTask& _task,
                                                const MapProjection& _projection) const {

    auto& task = static_cast<const DownloadTileTask&>(_task);

    std::shared_ptr<TileData> tileData = std::make_shared<TileData>();

    // Parse data into a JSON document
    const char* error;
    size_t offset;
    auto document = JsonParseBytes(task.rawTileData->data(), task.rawTileData->size(), &error, &offset);

    if (error) {
        LOGE("Json parsing failed on tile [%s]: %s (%u)", task.tileId().toString().c_str(), error, offset);
        return tileData;
    }

    // Transform JSON data into a TileData using TopoJson functions
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

    // Parse topology and transform
    auto topology = TopoJson::getTopology(document, projFn);

    // Parse each TopoJson object as a data layer
    auto objectsIt = document.FindMember("objects");
    if (objectsIt == document.MemberEnd()) { return tileData; }
    auto& objects = objectsIt->value;
    for (auto layer = objects.MemberBegin(); layer != objects.MemberEnd(); ++layer) {
        tileData->layers.push_back(TopoJson::getLayer(layer, topology, m_id));
    }

    // Discard JSON object and return TileData
    return tileData;

}

}
