#include "pbfParser.h"
#include "platform.h"
#include "tileID.h"

#include <sstream>
#include <fstream>

#include "protobufSource.h"

ProtobufSource::ProtobufSource() {
    m_urlTemplate = "http://vector.mapzen.com/osm/all/[z]/[x]/[y].mapbox";
}

std::shared_ptr<TileData> ProtobufSource::parse(const MapTile& _tile, const char* _rawData, const int _dataSize) {
    
    std::shared_ptr<TileData> tileData = std::make_shared<TileData>();
    
    protobuf::message item(_rawData, _dataSize);
    
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
