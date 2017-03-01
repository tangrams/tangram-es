#include "data/propertyItem.h"
#include "data/properties.h"
#include <algorithm>

namespace Tangram {

// Helper to cleanup double string values from trailing 0s
std::string doubleToString(double _doubleValue) {
    std::string value = std::to_string(_doubleValue);

    // Remove trailing '0's
    value.erase(value.find_last_not_of('0') + 1, std::string::npos);
    // If last value is '.', clear it too
    value.erase(value.find_last_not_of('.') + 1, std::string::npos);

    return value;
}

Properties::Properties() : sourceId(0) {}

Properties::~Properties() {}

Properties& Properties::operator=(Properties&& _other) {
    props = std::move(_other.props);
    sourceId = _other.sourceId;
    return *this;
}

void Properties::setSorted(std::vector<Item>&& _items) {
    props = std::move(_items);
}

const Value& Properties::get(const std::string& key) const {

    const auto it = std::find_if(props.begin(), props.end(),
                                 [&](const auto& item) {
                                     return item.key == key;
                                 });
    if (it == props.end()) {
        return NOT_A_VALUE;
    }

    // auto it = std::lower_bound(props.begin(), props.end(), key,
    //                            [](auto& item, auto& key) {
    //                                return keyComparator(item.key, key);
    //                            });
    // if (it == props.end() || it->key != key) {
    //     return NOT_FOUND;
    // }

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

bool Properties::getAsString(const std::string& key, std::string& value) const {
    auto& it = get(key);

    if (it.is<std::string>()) {
        value = it.get<std::string>();
        return true;
    } else if (it.is<double>()) {
        value = doubleToString(it.get<double>());
        return true;
    }

    return false;
}

std::string Properties::asString(const Value& value) const {
    if (value.is<std::string>()) {
        return value.get<std::string>();
    } else if (value.is<double>()) {
        return doubleToString(value.get<double>());
    }
    return "";
}

std::string Properties::getAsString(const std::string& key) const {

    return asString(get(key));

}

void Properties::sort() {
    std::sort(props.begin(), props.end());
}

void Properties::set(std::string key, std::string value) {

    auto it = std::lower_bound(props.begin(), props.end(), key,
                               [](auto& item, auto& key) {
                                   return keyComparator(item.key, key);
                               });

    if (it == props.end() || it->key != key) {
        props.emplace(it, std::move(key), std::move(value));
    } else {
        it->value = std::move(value);
    }
}

void Properties::set(std::string key, double value) {

    auto it = std::lower_bound(props.begin(), props.end(), key,
                               [](auto& item, auto& key) {
                                   return keyComparator(item.key, key);
                               });

    if (it == props.end() || it->key != key) {
        props.emplace(it, std::move(key), value);
    } else {
        it->value = value;
    }
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
