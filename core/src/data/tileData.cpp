#include "tileData.h"
#include <algorithm>

namespace Tangram {

int polygonSize(const Polygon& _polygon) {
    int size = 0;
    for (auto line : _polygon) {
        size += line.size();
    }
    return size;
}

Properties::Properties(std::vector<Item>&& _items) {
    typedef std::vector<Item>::iterator iter_t;

    props.reserve(_items.size());
    props.insert(props.begin(),
                 std::move_iterator<iter_t>(_items.begin()),
                 std::move_iterator<iter_t>(_items.end()));
    _items.clear();
    sort();
}

Properties& Properties::operator=(Properties&& _other) {
     props = std::move(_other.props);
     return *this;
}

const Value& Properties::get(const std::string& key) const {
    const static Value NOT_FOUND(none_type{});

    const auto it = std::lower_bound(props.begin(), props.end(), key,
                                     [](const auto& item, const auto& key) {
                                         return item.key < key;
                                     });

    if (it == props.end() || it->key != key) {
        return NOT_FOUND;
    }
    return it->value;
}

const std::string& Properties::getString(const std::string& key) const {
    const static std::string EMPTY_STRING = "";

    auto& it = get(key);
    if (it.is<std::string>()) {
        return it.get<std::string>();
    }
    return EMPTY_STRING;
}

std::string Properties::getAsString(const std::string& key) const {

    auto& it = get(key);
    if (it.is<std::string>()) {
        return it.get<std::string>();
    } else if (it.is<int64_t>()) {
        return std::to_string(it.get<int64_t>());
    } else if (it.is<float>()) {
        return std::to_string(it.get<float>());
    }
    return "";
}

void Properties::sort() {
    std::sort(props.begin(), props.end());
}

}
