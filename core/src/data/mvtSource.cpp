#include "data/mvtSource.h"
#include "data/tileData.h"
#include "tile/tileID.h"
#include "tile/tile.h"
#include "tile/tileTask.h"
#include "util/pbfParser.h"
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
    } catch(const std::invalid_argument& e) {
        LOGE("Cannot parse tile %s: %s", _task.tileId().toString().c_str(), e.what());
        return {};
    } catch(const std::runtime_error& e) {
        LOGE("Cannot parse tile %s: %s", _task.tileId().toString().c_str(), e.what());
        return {};
    } catch(...) {
        return {};
    }
    return tileData;
}

}
