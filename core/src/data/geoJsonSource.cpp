#include "geoJsonSource.h"

#include "util/geoJson.h"
#include "platform.h"
#include "tile/tileID.h"

#include "rapidjson/error/en.h"
#include "rapidjson/memorystream.h"
#include "rapidjson/encodings.h"
#include "rapidjson/encodedstream.h"

namespace Tangram {

GeoJsonSource::GeoJsonSource(const std::string& _name, const std::string& _urlTemplate) :
    DataSource(_name, _urlTemplate) {
}

std::shared_ptr<TileData> GeoJsonSource::parse(const Tile& _tile, std::vector<char>& _rawData) const {

    std::shared_ptr<TileData> tileData = std::make_shared<TileData>();

    // parse written data into a JSON object
    rapidjson::Document doc;

    rapidjson::MemoryStream ms(_rawData.data(), _rawData.size());
    rapidjson::EncodedInputStream<rapidjson::UTF8<char>, rapidjson::MemoryStream> is(ms);

    doc.ParseStream(is);

    if (doc.HasParseError()) {

        size_t offset = doc.GetErrorOffset();
        const char* error = rapidjson::GetParseError_En(doc.GetParseError());
        LOGE("Json parsing failed on tile [%d, %d, %d]: %s (%u)", _tile.getID().z, _tile.getID().x, _tile.getID().y, error, offset);
        return tileData;

    }

    // transform JSON data into a TileData using GeoJson functions
    for (auto layer = doc.MemberBegin(); layer != doc.MemberEnd(); ++layer) {
        tileData->layers.emplace_back(std::string(layer->name.GetString()));
        GeoJson::extractLayer(m_id, layer->value, tileData->layers.back(), _tile);
    }


    // Discard original JSON object and return TileData

    return tileData;

}

}
