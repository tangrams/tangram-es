#pragma once

#include "dataSource.h"
#include "tile.h"
#include "tileData.h"
#include "pbfParser.h"

namespace Tangram {

class VTMSource : public DataSource {
   protected:
    virtual std::shared_ptr<TileData> parse(const Tile& _tile, std::vector<char>& _rawData) const override;

    struct TagId {
        uint32_t key;
        uint32_t val;
        TagId(uint32_t key, uint32_t val) : key(key), val(val) {}
    };

    int zmin, zmax;
    bool s3db;

    bool extractFeature(protobuf::message it, GeometryType geomType,
                        const std::vector<TagId>& tags,
                        const std::vector<std::string>& keys,
                        const std::vector<std::string>& vals,
                        TileData& tileData, const Tile& _tile) const;

    void readTags(protobuf::message it, std::vector<TagId>& tags) const;

   public:
    VTMSource(int zmin, int zmax, bool s3db, const std::string& _name, const std::string& _urlTemplate);

    int minZoom() const {
        return zmin;
    };
    int maxZoom() const {
        return zmax;
    };

   virtual bool loadTileData(std::shared_ptr<TileTask>&& _task, TileTaskCb _cb) override;

};

}
