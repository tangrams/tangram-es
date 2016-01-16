#include "mvtSource.h"

#include "tileData.h"
#include "tile/tileID.h"
#include "tile/tile.h"
#include "tile/tileTask.h"
#include "util/pbfParser.h"
#include "platform.h"

#include <sstream>
#include <fstream>

namespace Tangram {

#define TAG_TILE_LAYER 3
#define TAG_LAYER_NAME 1

MVTSource::MVTSource(const std::string& _name, const std::string& _urlTemplate, int32_t _maxZoom) :
    DataSource(_name, _urlTemplate, _maxZoom) {
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
                //layerMsg.skip();
                break;
            }

            PbfParser::extractLayer(ctx, layerMsg, _sink);
            break;
        }

    }
    return true;
#if 0
    while(item.next()) {
        if(item.tag == 3) {
            protobuf::message layerMsg = item.getMessage();
            protobuf::message layerItr = layerMsg;
            while (layerItr.next()) {
                if (layerItr.tag == 1) {
                    if (_sink.beginLayer(layerItr.string())) {
                        PbfParser::extractLayer(ctx, layerMsg, _sink);
                    } else {
                        layerItr.skip();
                    }
                    PbfParser::extractLayer(ctx, layerMsg, _sink);
                } else {
                    layerItr.skip();
                }
            }
        } else {
            item.skip();
        }
    }
    return true;
#endif
}

}
