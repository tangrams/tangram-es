#include "pbfParser.h"
#include "platform.h"
#include "tileID.h"

#include <sstream>
#include <fstream>

#include "protobufSrc.h"

MapboxProtoBuffSrc::MapboxProtoBuffSrc() {
    m_urlTemplate = "http://vector.mapzen.com/osm/all/[z]/[x]/[y].mapbox";
}

std::shared_ptr<TileData> MapboxProtoBuffSrc::parse(const MapTile& _tile, std::stringstream& _in) {
    
    std::shared_ptr<TileData> tileData = std::make_shared<TileData>();
    
    std::string buffer(std::istreambuf_iterator<char>(_in.rdbuf()), (std::istreambuf_iterator<char>()));
    
    protobuf::message item(buffer.data(), buffer.size());
    
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
