#pragma once

#include "data/properties.h"
#include "data/geometry.h"

#include <vector>
#include <string>


namespace Tangram {

/* A <Feature> contains a <Properties> struct describing the feature, and a
 * <Geometry> corresponding to the feature's geometry type.
 *
 * A <Properties> contains a sorted vector of key-value pairs storing the
 * properties of a <Feature>
 *
 * <Geometry> Tile Coordinates:
 *
 * A point in the geometry of a tile is represented with 32-bit floating point
 * x, y, and z coordinates. Coordinates represent normalized displacement from
 * the origin (i.e. lower-left corner) of a tile.
 *
 * (0.0, 1.0) ---------- (1.0, 1.0)
 *           |          |             N
 *           ^ +y       |          W <|> E
 *           |          |             S
 *           |    +x    |
 * (0.0, 0.0) ----->---- (1.0, 0.0)
 *
 * Coordinates that fall outside the range [0.0, 1.0] are permissible, as tile
 * servers may choose not to clip certain geometries to tile boundaries, but
 * these points are clipped in the client-side geometry processing.
 *
 * Z coordinates are expected to be normalized to the same scale as x, y
 * coordinates.
 */
struct Feature {
    Feature() {}
    Feature(int32_t _sourceId) { props.sourceId = _sourceId; }

    Properties props;
    Geometry<Point> geometry;

    auto& points() const { return geometry.points(); }
    auto lines() const { return geometry.lines(); }
    auto polygons() const { return geometry.polygons(); }
};

/* Callback interface for <DataSource>s */
struct TileDataSink {
    virtual bool beginLayer(const std::string& _layer) = 0;
    virtual bool matchFeature(const Feature& _feature) = 0;
    virtual void addFeature(const Feature& _feature) = 0;
};

/*
 * Unused - but may be handy for writing tests.
 *
 * <TileData> contains a collection of <Layer>s
 * (following the MVT spec)
 *
 * A <Layer> contains a name and a collection of <Feature>s.
 *
 * <Feature>s are modeled after the GeoJSON spec:
 * http://geojson.org/geojson-spec.html
 */
struct Layer {
    Layer(const std::string& _name) : name(_name) {}

    std::string name;

    std::vector<Feature> features;

    void process(TileDataSink& _sink) {
        for (auto& feat : features) {
            if (_sink.matchFeature(feat)) {
                _sink.addFeature(feat);
            }
        }
    }
};

struct TileData {

    std::vector<Layer> layers;

    void process(TileDataSink& _sink) {
        for (auto& layer : layers) {
            if (_sink.beginLayer(layer.name)) {
                layer.process(_sink);
            }
        }
    }
};

}
