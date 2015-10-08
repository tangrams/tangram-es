#pragma once

#include "../../../src/data/properties.h"

#include <memory>

namespace mapbox {
namespace util {
namespace geojsonvt {

struct Tags {
    std::shared_ptr<Tangram::Properties> map;

    void emplace(std::string key, std::string value) {
        map->add(std::move(key), std::move(value));
    }

};

} // namespace geojsonvt
} // namespace util
} // namespace mapbox
