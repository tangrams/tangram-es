#include "geoJson.h"
#include "platform.h"
#include "tileID.h"

#include "mapzenVectorTileJson.h"


MapzenVectorTileJson::MapzenVectorTileJson() {
    m_urlTemplate = "http://vector.mapzen.com/osm/all/[z]/[x]/[y].json";
}

std::shared_ptr<TileData> MapzenVectorTileJson::parse(const MapTile& _tile, std::stringstream& _in) {
    
    std::shared_ptr<TileData> tileData = std::make_shared<TileData>();
    
    // parse written data into a JSON object
    Json::Reader jsonReader;
    Json::Value jsonValue;
    
    if (! jsonReader.parse(_in, jsonValue)) {
        
        logMsg("Json parsing failed on tile [%d, %d, %d]\n", _tile.getID().z, _tile.getID().x, _tile.getID().y);
        return tileData;
        
    }
    
    // transform JSON data into a TileData using GeoJson functions
    for (const auto& layerName : jsonValue.getMemberNames()) {
        tileData->layers.emplace_back(layerName);
        GeoJson::extractLayer(jsonValue[layerName], tileData->layers.back(), _tile);
    }
    
    
    // Discard original JSON object and return TileData
    
    return tileData;
    
}
