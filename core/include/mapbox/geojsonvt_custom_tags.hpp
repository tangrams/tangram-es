#pragma once

#include "data/properties.h"
#include "data/propertyItem.h"

#include <memory>

namespace mapbox {
namespace util {
namespace geojsonvt {

struct Tags {
    Tags() : map(std::make_shared<Tangram::Properties>()){}

    Tags(std::shared_ptr<Tangram::Properties> _props) :
        map(std::move(_props)){}

    Tags(const Tangram::Properties& _props) :
        map(std::make_shared<Tangram::Properties>(_props)){}

    std::shared_ptr<Tangram::Properties> map;

    void emplace(std::string key, std::string value) {
        map->set(std::move(key), std::move(value));
    }

};

} // namespace geojsonvt
} // namespace util
} // namespace mapbox
