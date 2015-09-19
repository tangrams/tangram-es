#pragma once

#include <vector>
#include <string>

namespace Tangram {

class Value;
struct PropertyItem;

struct Properties {
    using Item = PropertyItem;

    Properties();
    ~Properties();

    Properties(const Properties& _other) = default;
    Properties(std::vector<Item>&& _items);
    Properties& operator=(const Properties& _other) = default;
    Properties& operator=(Properties&& _other);

    const Value& get(const std::string& key) const;

    void sort();

    void clear();

    bool contains(const std::string& key) const;

    bool getNumeric(const std::string& key, double& value) const;

    double getNumeric(const std::string& key) const;

    bool getString(const std::string& key, std::string& value) const;

    const std::string& getString(const std::string& key) const;

    std::string asString(const Value& value) const;

    std::string getAsString(const std::string& key) const;

    std::string toJson() const;

    void add(std::string key, std::string value);
    void add(std::string key, float value);

    // template <typename... Args> void add(std::string key, Args&&... args) {
    //     props.emplace_back(std::move(key), Value{std::forward<Args>(args)...});
    //     sort();
    // }

    const std::vector<Item>& items() const { return props; }

    int32_t sourceId;

private:
    std::vector<Item> props;
};

}
