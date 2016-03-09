#include "mvtSource.h"

#include "tileData.h"
#include "tile/tileID.h"
#include "tile/tile.h"
#include "tile/tileTask.h"
#include "util/pbfParser.h"
#include "platform.h"

namespace Tangram {

#define TAG_TILE_LAYER 3
#define TAG_LAYER_NAME 1

MVTSource::MVTSource(const std::string& _name, const std::string& _urlTemplate,
                     int32_t _minDisplayZoom, int32_t _maxDisplayZoom, int32_t _maxZoom) :
    DataSource(_name, _urlTemplate, _minDisplayZoom, _maxDisplayZoom, _maxZoom) {
}

bool MVTSource::process(const TileTask& _task, const MapProjection& _projection,
                        TileDataSink& _sink) const {


    auto& task = static_cast<const DownloadTileTask&>(_task);

    protobuf::message tileMsg(task.rawTileData->data(), task.rawTileData->size());
    PbfParser::ParserContext ctx(m_id);

    while (tileMsg.next()) {
        if (tileMsg.tag != TAG_TILE_LAYER) {
            tileMsg.skip();
            continue;
        }
        protobuf::message layerMsg = tileMsg.getMessage();
        protobuf::message layerItr = layerMsg;

        while (layerItr.next()) {

            if (layerItr.tag != TAG_LAYER_NAME) {
                layerItr.skip();
                continue;
            }
            auto layerName = layerItr.string();

            if (!_sink.beginLayer(layerName)){
                break;
            }

            PbfParser::extractLayer(ctx, layerMsg, _sink);
            break;
        }

    }
    return true;
}

}
