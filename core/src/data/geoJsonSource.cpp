#include "geoJson.h"
#include "platform.h"
#include "tileID.h"

#include "geoJsonSource.h"


GeoJsonSource::GeoJsonSource() {
    m_urlTemplate = "http://vector.mapzen.com/osm/all/[z]/[x]/[y].json";
}

std::shared_ptr<TileData> GeoJsonSource::parse(const MapTile& _tile, std::vector<char>& _rawData) {

    std::shared_ptr<TileData> tileData = std::make_shared<TileData>();
    
    // parse written data into a JSON object
    rapidjson::Document doc;
    doc.Parse(_rawData.data());

    if (doc.HasParseError()) {

        logMsg("Json parsing failed on tile [%d, %d, %d]\n", _tile.getID().z, _tile.getID().x, _tile.getID().y);
        return tileData;

    }

    // transform JSON data into a TileData using GeoJson functions
    for (auto layer = doc.MemberBegin(); layer != doc.MemberEnd(); ++layer) {
        tileData->layers.emplace_back(std::string(layer->name.GetString()));
        GeoJson::extractLayer(layer->value, tileData->layers.back(), _tile);
    }


    // Discard original JSON object and return TileData

    return tileData;

}

