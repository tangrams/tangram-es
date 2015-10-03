#pragma once

#include "util/variant.h"
#include <vector>

namespace Tangram {

struct Properties {

    struct Item {
        Item(std::string _key, Value _value) :
            key(std::move(_key)), value(std::move(_value)) {}

        std::string key;
        Value value;
        bool operator<(const Item& _rhs) const { return key < _rhs.key; }
    };

    Properties() {}
    Properties(const Properties& _other) = default;
    Properties(std::vector<Item>&& _items);
    Properties& operator=(const Properties& _other) = default;
    Properties& operator=(Properties&& _other);

    const Value& get(const std::string& key) const;

    void sort();

    void clear() { props.clear(); }

    bool contains(const std::string& key) const {
        return !get(key).is<none_type>();
    }

    bool getNumeric(const std::string& key, double& value) const {
        auto& it = get(key);
        if (it.is<float>()) {
            value = it.get<float>();
            return true;
        } else if (it.is<int64_t>()) {
            value = it.get<int64_t>();
            return true;
        }
        return false;
    }

    double getNumeric(const std::string& key) const {
        auto& it = get(key);
        if (it.is<float>()) {
            return it.get<float>();
        } else if (it.is<int64_t>()) {
            return it.get<int64_t>();
        }
        return 0;
    }
    bool getString(const std::string& key, std::string& value) const {
        auto& it = get(key);
        if (it.is<std::string>()) {
            value = it.get<std::string>();
            return true;
        }
        return false;
    }

    const std::string& getString(const std::string& key) const;

    std::string asString(const Value& value) const;

    std::string getAsString(const std::string& key) const;

    std::string toJson() const;

    template <typename... Args> void add(std::string key, Args&&... args) {
        props.emplace_back(std::move(key), Value{std::forward<Args>(args)...});
        sort();
    }

    const auto& items() const { return props; }

    int32_t sourceId;

private:
    std::vector<Item> props;
};

}
