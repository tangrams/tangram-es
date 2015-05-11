#include "pbfParser.h"
#include "platform.h"
#include "tileID.h"

#include <sstream>
#include <fstream>

#include "mvtSource.h"

MVTSource::MVTSource() {
    m_urlTemplate = "http://vector.mapzen.com/osm/all/[z]/[x]/[y].mapbox";
}

std::shared_ptr<TileData> MVTSource::parse(const MapTile& _tile, std::vector<char>& _rawData) {
    
    std::shared_ptr<TileData> tileData = std::make_shared<TileData>();
    
    protobuf::message item(_rawData.data(), _rawData.size());

    while(item.next()) {
        if(item.tag == 3) {
            protobuf::message layerMsg = item.getMessage();
            protobuf::message layerItr = layerMsg;
            while (layerItr.next()) {
                if (layerItr.tag == 1) {
                    auto layerName = layerItr.string();
                    tileData->layers.emplace_back(layerName);
                    PbfParser::extractLayer(layerMsg, tileData->layers.back(), _tile);
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

