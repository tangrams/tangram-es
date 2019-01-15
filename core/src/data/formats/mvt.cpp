#include "data/formats/mvt.h"
#include "data/propertyItem.h"
#include "tile/tile.h"
#include "tile/tileTask.h"
#include "log.h"
#include "platform.h"
#include "util/geom.h"

#include <algorithm>
#include <iterator>

#define LAYER 3

#define FEATURE_ID 1
#define FEATURE_TAGS 2
#define FEATURE_TYPE 3
#define FEATURE_GEOM 4

#define LAYER_NAME 1
#define LAYER_FEATURE 2
#define LAYER_KEY 3
#define LAYER_VALUE 4
#define LAYER_TILE_EXTENT 5

namespace Tangram {


void Mvt::getGeometry(ParserContext& _ctx, protobuf::message _geomIn) {

    auto& geometry = _ctx.geometry;
    geometry.coordinates.clear();
    geometry.sizes.clear();

    GeomCmd cmd = GeomCmd::moveTo;
    uint32_t cmdRepeat = 0;

    double invTileExtent = (1.0/(_ctx.tileExtent-1.0));

    int64_t x = 0;
    int64_t y = 0;

    size_t numCoordinates = 0;

    while(_geomIn.getData() < _geomIn.getEnd()) {

        if(cmdRepeat == 0) { // get new command, length and parameters..
            uint32_t cmdData = static_cast<uint32_t>(_geomIn.varint());
            cmd = static_cast<GeomCmd>(cmdData & 0x7); //first 3 bits of the cmdData
            cmdRepeat = cmdData >> 3; //last 5 bits
        }

        if(cmd == GeomCmd::moveTo || cmd == GeomCmd::lineTo) { // get parameters/points
            // if cmd is move then move to a new line/set of points and save this line
            if(cmd == GeomCmd::moveTo) {
                if (geometry.coordinates.size() > 0) {
                    geometry.sizes.push_back(numCoordinates);
                }
                numCoordinates = 0;
            }

            x += _geomIn.svarint();
            y += _geomIn.svarint();

            // bring the points in 0 to 1 space
            Point p;
            p.x = invTileExtent * (double)x;
            p.y = invTileExtent * (double)(_ctx.tileExtent - y);

            if (numCoordinates == 0 || geometry.coordinates.back() != p) {
                geometry.coordinates.push_back(p);
                numCoordinates++;
            }
        } else if(cmd == GeomCmd::closePath) {
            // end of a polygon, push first point in this line as last and push line to poly
            geometry.coordinates.push_back(geometry.coordinates[geometry.coordinates.size() - numCoordinates]);
            geometry.sizes.push_back(numCoordinates + 1);
            numCoordinates = 0;
        }

        cmdRepeat--;
    }

    // Enter the last line
    if (numCoordinates > 0) {
        geometry.sizes.push_back(numCoordinates);
    }
}

void Mvt::getFeature(ParserContext& _ctx, std::vector<Feature>& features, protobuf::message _featureIn) {


    GeometryType geometryType;

    auto& featureTags = _ctx.featureTags;
    featureTags.clear();
    featureTags.assign(_ctx.keys.size(), -1);

    while(_featureIn.next()) {
        switch(_featureIn.tag) {
            case FEATURE_ID:
                // ignored for now, also not used in json parsing
                _featureIn.skip();
                break;

            case FEATURE_TAGS: {
                protobuf::message tagsMsg = _featureIn.getMessage();

                while(tagsMsg) {
                    auto tagKey = tagsMsg.varint();

                    if(_ctx.keys.size() <= tagKey) {
                        LOGE("accessing out of bound key");
                        return;
                    }

                    if(!tagsMsg) {
                        LOGE("uneven number of feature tag ids");
                        return;
                    }

                    auto valueKey = tagsMsg.varint();
                    // Check if the property should be dropped
                    if (_ctx.keys[tagKey].empty()) {
                        continue;
                    }
                    if( _ctx.values.size() <= valueKey ) {
                        LOGE("accessing out of bound values");
                        return;
                    }
                    featureTags[tagKey] = valueKey;
                }
                break;
            }
            case FEATURE_TYPE:
                geometryType = (GeometryType)_featureIn.varint();
                break;
            // Actual geometry data
            case FEATURE_GEOM:
                getGeometry(_ctx, _featureIn.getMessage());
                break;

            default:
                _featureIn.skip();
                break;
        }
    }

    int id = -1;
    bool newFeature = true;

    if (_ctx.mergeFeatures && !_ctx.featureMap.empty()) {
        // Check the previous added feature first
        auto& prev = _ctx.featureMap.back();
        if (prev.first == featureTags &&
            features[prev.second].geometryType == geometryType) {
            id = prev.second;
            newFeature = false;
            LOG("HIT 1");
        }
        if (newFeature) {
            auto& prev = _ctx.featureMap[_ctx.lastFound];
            if (prev.first == featureTags &&
                features[prev.second].geometryType == geometryType) {
                id = prev.second;
                newFeature = false;
                LOG("HIT 2");
            }
        }
        if (newFeature) {
            size_t l = 0;
            for (auto& f : _ctx.featureMap) {
                if (f.first == featureTags &&
                    features[f.second].geometryType == geometryType) {
                    id = f.second;
                    _ctx.lastFound = l;
                    newFeature = false;
                    break;
                }
                l++;
            }
        }
    }
    if (newFeature) {
        id = features.size();
        features.emplace_back(_ctx.sourceId);

        if (_ctx.mergeFeatures) {
            _ctx.featureMap.emplace_back(featureTags, id);
        }

        //LOGE("ADD %d", id);
    } else {
        LOGE("USE SAME PROPS / %d %d / %d", id, _ctx.featureMap.size()-1, _ctx.featureMap.size()-1-id);
        _ctx.featureMerged++;

    }

    _ctx.featureSum++;

    auto& feature = features[id];
    feature.geometryType = geometryType;

    if (newFeature) {
        std::vector<Properties::Item> properties;
        properties.reserve(featureTags.size());

        for (int tagKey : _ctx.orderedKeys) {
            int tagValue = featureTags[tagKey];
            if (tagValue >= 0) {
                properties.emplace_back(_ctx.keys[tagKey], _ctx.values[tagValue]);
            }
        }
        feature.props.setSorted(std::move(properties));
    }

    auto& geometry = _ctx.geometry;

    switch(geometryType) {
        case GeometryType::points:
            feature.points.insert(feature.points.end(),
                                  geometry.coordinates.begin(),
                                  geometry.coordinates.end());
            break;

        case GeometryType::lines:
        {
            auto pos = geometry.coordinates.begin();
            for (int length : geometry.sizes) {
                if (length == 0) { continue; }
                Line line;
                line.reserve(length);
                line.insert(line.begin(), pos, pos + length);
                pos += length;
                feature.lines.emplace_back(std::move(line));
            }
            break;
        }
        case GeometryType::polygons:
        {
            auto pos = geometry.coordinates.begin();
            auto rpos = geometry.coordinates.rend();
            for (int length : geometry.sizes) {
                if (length == 0) { continue; }
                float area = signedArea(pos, pos + length);
                if (area == 0) {
                    pos += length;
                    rpos -= length;
                    continue;
                }
                int winding = area > 0 ? 1 : -1;
                // Determine exterior winding from first polygon.
                if (_ctx.winding == 0) {
                    _ctx.winding = winding;
                }
                Line line;
                line.reserve(length);
                if (_ctx.winding > 0) {
                    line.insert(line.end(), pos, pos + length);
                } else {
                    line.insert(line.end(), rpos - length, rpos);
                }
                pos += length;
                rpos -= length;
                if (winding == _ctx.winding || feature.polygons.empty()) {
                    // This is an exterior polygon.
                    feature.polygons.emplace_back();
                }
                feature.polygons.back().push_back(std::move(line));
            }
            break;
        }
        case GeometryType::unknown:
            break;
        default:
            break;
    }
}

Layer Mvt::getLayer(ParserContext& _ctx, protobuf::message _layerIn,
                    const TileSource::PropertyFilter& filter) {

    Layer layer("");
    _ctx.keys.clear();
    _ctx.values.clear();
    _ctx.featureMsgs.clear();
    _ctx.featureMap.clear();
    _ctx.mergeFeatures = !filter.drop.empty();

    bool lastWasFeature = false;
    size_t numFeatures = 0;
    protobuf::message featureItr;

    // Iterate layer to populate featureMsgs, keys and values
    while(_layerIn.next()) {

        switch(_layerIn.tag) {
            case LAYER_NAME: {
                layer.name = _layerIn.string();
                break;
            }
            case LAYER_FEATURE: {
                numFeatures++;
                if (!lastWasFeature) {
                    _ctx.featureMsgs.push_back(_layerIn);
                    lastWasFeature = true;
                }
                _layerIn.skip();
                continue;
            }
            case LAYER_KEY: {
                std::string key = _layerIn.string();
                // Check whether the key must be kept
                if (std::find(std::begin(filter.keep), std::end(filter.keep), key) == std::end(filter.keep)) {
                    // Check whether the key should be dropped
                    if (std::find_if(std::begin(filter.drop), std::end(filter.drop),
                                     [&](auto& k) {
                                         if (k.back() == '*' && key.length() >= k.length()-1) {
                                             int n = std::strncmp(key.c_str(), k.c_str(), k.length()-1);
                                             //LOG("check %s / %d / %d", k.c_str(), n, k.length()-1);
                                             return  n == 0;
                                         } else {
                                             return key == k;
                                         }}) != std::end(filter.drop)) {

                        LOG("drop key: %s", key.c_str());
                        key = "";
                        } else {
                        LOG(">>> keep key: %s", key.c_str());
                    }
                    } else {
                    LOG(">>> keep key: %s", key.c_str());
                }
                _ctx.keys.emplace_back(std::move(key));
                break;
            }
            case LAYER_VALUE: {
                protobuf::message valueItr = _layerIn.getMessage();

                while (valueItr.next()) {
                    switch (valueItr.tag) {
                        case 1: // string value
                            _ctx.values.push_back(valueItr.string());
                            break;
                        case 2: // float value
                            _ctx.values.push_back(valueItr.float32());
                            break;
                        case 3: // double value
                            _ctx.values.push_back(valueItr.float64());
                            break;
                        case 4: // int value
                            _ctx.values.push_back(valueItr.int64());
                            break;
                        case 5: // uint value
                            _ctx.values.push_back(valueItr.varint());
                            break;
                        case 6: // sint value
                            _ctx.values.push_back(valueItr.int64());
                            break;
                        case 7: // bool value
                            _ctx.values.push_back(valueItr.boolean());
                            break;
                        default:
                            _ctx.values.push_back(none_type{});
                            valueItr.skip();
                            break;
                    }
                }
                break;
            }
            case LAYER_TILE_EXTENT:
                _ctx.tileExtent = static_cast<int>(_layerIn.int64());
                break;

            default: // skip
                _layerIn.skip();
                break;
        }
        lastWasFeature = false;
    }
    LOG(">>>>>>>>>>>>>>>>>>>>>>>>>>>>> layer: %s", layer.name.c_str());

    if (_ctx.featureMsgs.empty()) { return layer; }

    //// Assign ordering to keys for faster sorting
    _ctx.orderedKeys.clear();
    _ctx.orderedKeys.reserve(_ctx.keys.size());
    // assign key ids
    for (int i = 0, n = _ctx.keys.size(); i < n; i++) {
        _ctx.orderedKeys.push_back(i);
    }
    // sort by Property key ordering
    std::sort(_ctx.orderedKeys.begin(), _ctx.orderedKeys.end(),
              [&](int a, int b) {
                  return Properties::keyComparator(_ctx.keys[a], _ctx.keys[b]);
              });

    layer.features.reserve(numFeatures);
    for (auto& featureItr : _ctx.featureMsgs) {
        do {
            auto featureMsg = featureItr.getMessage();

            getFeature(_ctx, layer.features, featureMsg);

        } while (featureItr.next() && featureItr.tag == LAYER_FEATURE);
    }

    return layer;
}

std::shared_ptr<TileData> Mvt::parseTile(const TileTask& _task, int32_t _sourceId,
                                         const TileSource::PropertyFilter& filter) {

    auto tileData = std::make_shared<TileData>();

    auto& task = static_cast<const BinaryTileTask&>(_task);

    protobuf::message item(task.rawTileData->data(), task.rawTileData->size());
    ParserContext ctx(_sourceId);

    try {
        while(item.next()) {
            if(item.tag == LAYER) {
                tileData->layers.push_back(getLayer(ctx, item.getMessage(), filter));
            } else {
                item.skip();
            }
        }
    } catch(const std::invalid_argument& e) {
        LOGE("Cannot parse tile %s: %s", _task.tileId().toString().c_str(), e.what());
        return {};
    } catch(const std::runtime_error& e) {
        LOGE("Cannot parse tile %s: %s", _task.tileId().toString().c_str(), e.what());
        return {};
    } catch(...) {
        return {};
    }
    LOGE("SUM:%d REUSE:%d (%f)", ctx.featureSum, ctx.featureMerged, float(ctx.featureSum) / ctx.featureMerged);

    return tileData;
}

}
