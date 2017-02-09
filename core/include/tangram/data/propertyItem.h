#pragma once

#include "util/variant.h"

namespace Tangram {

struct PropertyItem {
    PropertyItem(std::string _key, Value _value) :
        key(std::move(_key)), value(std::move(_value)) {}

    std::string key;
    Value value;
    bool operator<(const PropertyItem& _rhs) const {
        return key.size() == _rhs.key.size()
            ? key < _rhs.key
            : key.size() < _rhs.key.size();
    }
};

}
