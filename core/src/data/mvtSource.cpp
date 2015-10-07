#include "mvtSource.h"

#include "util/pbfParser.h"
#include "platform.h"
#include "tile/tileID.h"

#include <sstream>
#include <fstream>

namespace Tangram {

MVTSource::MVTSource(const std::string& _name, const std::string& _urlTemplate) :
    DataSource(_name, _urlTemplate) {
}

std::shared_ptr<TileData> MVTSource::parse(const Tile& _tile, std::vector<char>& _rawData) const {

    std::shared_ptr<TileData> tileData = std::make_shared<TileData>();

    protobuf::message item(_rawData.data(), _rawData.size());
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
