#include "pbfParser.h"
#include "platform.h"
#include "tileID.h"

#include <sstream>
#include <fstream>

#include "protobuffSrc.h"

MapboxProtoBuffSrc::MapboxProtoBuffSrc() {
    m_urlTemplate = "http://vector.mapzen.com/osm/all/[z]/[x]/[y].mapbox";
}

std::shared_ptr<TileData> MapboxProtoBuffSrc::parse(const MapTile& _tile, std::stringstream& _in) {
    
    std::shared_ptr<TileData> tileData = std::make_shared<TileData>();
    
    std::string filename("/Users/Varun/Downloads/0.pbf");
    std::ifstream stream(filename.c_str(),std::ios_base::in|std::ios_base::binary);
    if (!stream.is_open())
    {
        throw std::runtime_error("could not open: '" + filename + "'");
    }
    std::string buffer(std::istreambuf_iterator<char>(stream.rdbuf()),(std::istreambuf_iterator<char>()));
    stream.close();
    
    //std::string buffer(std::istreambuf_iterator<char>(_in.rdbuf()), (std::istreambuf_iterator<char>()));
    
    protobuf::message item(buffer.data(), buffer.size());
    
    while(item.next()) {
        if(item.tag == 3) {
            protobuf::message layerMsg = item.getMessage();
            protobuf::message layerItr = layerMsg;
            while (layerItr.next()) {
                if (layerItr.tag == 1) {
                    auto layerName = layerItr.string();
                    logMsg("Layer string: %s\n", layerName.c_str());
                    tileData->layers.emplace_back(layerName);
                    PbfParser::extractLayer(layerMsg, tileData->layers.back(), _tile);
                } else {
                    layerMsg.skip();
                }
            }
        } else {
            item.skip();
        }
    }
    return tileData;
    
}
