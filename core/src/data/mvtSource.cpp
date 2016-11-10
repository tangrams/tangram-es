#include "mvtSource.h"

#include "tileData.h"
#include "tile/tileID.h"
#include "tile/tile.h"
#include "tile/tileTask.h"
#include "util/pbfParser.h"
#include "platform.h"
#include "log.h"

namespace Tangram {

std::shared_ptr<TileData> MVTSource::parse(const TileTask& _task, const MapProjection& _projection) const {

    auto tileData = std::make_shared<TileData>();

    auto& task = static_cast<const BinaryTileTask&>(_task);

    protobuf::message item(task.rawTileData->data(), task.rawTileData->size());
    PbfParser::ParserContext ctx(m_id);

    try {
        while(item.next()) {
            if(item.tag == 3) {
                tileData->layers.push_back(PbfParser::getLayer(ctx, item.getMessage()));
            } else {
                item.skip();
            }
        }
    } catch(std::runtime_error e) {
        LOGE("%s", e.what());
    }
    return tileData;
}

}
