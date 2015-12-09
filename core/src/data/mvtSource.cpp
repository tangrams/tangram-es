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


MVTSource::MVTSource(const std::string& _name, const std::string& _urlTemplate) :
    DataSource(_name, _urlTemplate) {
}

std::shared_ptr<TileData> MVTSource::parse(const TileTask& _task, const MapProjection& _projection) const {

    auto tileData = std::make_shared<TileData>();

    auto& task = dynamic_cast<const DownloadTileTask&>(_task);

    protobuf::message item(task.rawTileData->data(), task.rawTileData->size());
    PbfParser::ParserContext ctx(m_id);

    while(item.next()) {
        if(item.tag == 3) {
            protobuf::message layerMsg = item.getMessage();
            protobuf::message layerItr = layerMsg;
            while (layerItr.next()) {
                if (layerItr.tag == 1) {
                    auto layerName = layerItr.string();
                    tileData->layers.emplace_back(layerName);
                    PbfParser::extractLayer(ctx, layerMsg, tileData->layers.back());
                } else {
                    layerItr.skip();
                }
            }
        } else {
            item.skip();
        }
    }
    return tileData;
}

}
