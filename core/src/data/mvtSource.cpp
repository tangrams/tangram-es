#include "data/mvtSource.h"
#include "data/tileData.h"
#include "tile/tileID.h"
#include "tile/tile.h"
#include "tile/tileTask.h"
#include "util/pbfParser.h"
#include "log.h"

namespace Tangram {

#define TAG_TILE_LAYER 3
#define TAG_LAYER_NAME 1

bool MVTSource::process(const TileTask& _task, const MapProjection& _projection,
                        TileDataSink& _sink) const {

    auto& task = static_cast<const BinaryTileTask&>(_task);

    protobuf::message tileMsg(task.rawTileData->data(), task.rawTileData->size());
    PbfParser::ParserContext ctx(m_id);

    try {
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
    } catch(const std::invalid_argument& e) {
        LOGE("Cannot parse tile %s: %s", _task.tileId().toString().c_str(), e.what());
        return false;
    } catch(const std::runtime_error& e) {
        LOGE("Cannot parse tile %s: %s", _task.tileId().toString().c_str(), e.what());
        return false;
    } catch(...) {
        return false;
    }

    return true;
}
}
