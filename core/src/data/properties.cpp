#include "propertyItem.h"
#include "properties.h"
#include <algorithm>

namespace Tangram {

Properties::Properties() {}

Properties::~Properties() {}

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
                                         if (item.key.size() == key.size()) {
                                             return item.key < key;
                                         } else {
                                             return item.key.size() < key.size();
                                         }
                                     });

    if (it == props.end() || it->key != key) {
        return NOT_FOUND;
    }
    return it->value;
}

void Properties::clear() { props.clear(); }

bool Properties::contains(const std::string& key) const {
    return !get(key).is<none_type>();
}

bool Properties::getNumber(const std::string& key, double& value) const {
    auto& it = get(key);
    if (it.is<double>()) {
        value = it.get<double>();
        return true;
    }
    return false;
}

double Properties::getNumber(const std::string& key) const {
    auto& it = get(key);
    if (it.is<double>()) {
        return it.get<double>();
    }
    return 0;
}

bool Properties::getString(const std::string& key, std::string& value) const {
    auto& it = get(key);
    if (it.is<std::string>()) {
        value = it.get<std::string>();
        return true;
    }
    return false;
}

const std::string& Properties::getString(const std::string& key) const {
    const static std::string EMPTY_STRING = "";

    auto& it = get(key);
    if (it.is<std::string>()) {
        return it.get<std::string>();
    }
    return EMPTY_STRING;
}

std::string Properties::asString(const Value& value) const {
    if (value.is<std::string>()) {
        return value.get<std::string>();
    } else if (value.is<double>()) {
        return std::to_string(value.get<double>());
    }
    return "";
}

std::string Properties::getAsString(const std::string& key) const {

    return asString(get(key));

}

void Properties::sort() {
    std::sort(props.begin(), props.end());
}

void Properties::add(std::string key, std::string value) {
    props.emplace_back(std::move(key), Value{std::move(value)});
    sort();
}
void Properties::add(std::string key, double value) {
    props.emplace_back(std::move(key), Value{value});
    sort();
}

std::string Properties::toJson() const {

    std::string json = "{ ";

    for (const auto& item : props) {
        bool last = (&item == &props.back());
        json += "\"" + item.key + "\": \"" + asString(item.value) + (last ? "\"" : "\",");
    }

    json += " }";

    return json;
}

}
