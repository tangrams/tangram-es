#pragma once

#include "dataSource.h"
#include "mapbox/geojsonvt/geojsonvt.hpp"
#include "types.h"

namespace Tangram {

using GeoJSONVT = mapbox::util::geojsonvt::GeoJSONVT;

class ClientGeoJsonSource : public DataSource {

public:

    ClientGeoJsonSource(const std::string& _name, const std::string& _url);

    void setData(const std::string& _data);
    void addData(GeoPoint* _points, int length);
    void addData(GeoLine* _lines, int length);
    void addData(GeoPolygon* _polygons, int length);

    virtual bool loadTileData(std::shared_ptr<TileTask>&& _task, TileTaskCb _cb) override;
    virtual bool getTileData(std::shared_ptr<TileTask>& _task) override;
    virtual void cancelLoadingTile(const TileID& _tile) override {};
    virtual void clearData() override;

protected:

    virtual std::shared_ptr<TileData> parse(const Tile& _tile, std::vector<char>& _rawData) const override;

    std::unique_ptr<GeoJSONVT> m_store;
    std::vector<mapbox::util::geojsonvt::ProjectedFeature> m_features;

};

}
