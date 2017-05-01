#pragma once

#include <string>
#include <vector>

namespace Tangram {

class Value;
struct PropertyItem;

struct Properties {
    using Item = PropertyItem;

    Properties();
    ~Properties();

    Properties(const Properties& _other) = default;
    Properties(Properties&& _other) = default;
    Properties(std::vector<Item>&& _items);
    Properties& operator=(const Properties& _other) = default;
    Properties& operator=(Properties&& _other);

    const Value& get(const std::string& key) const;

    void sort();

    void clear();

    bool contains(const std::string& key) const;

    bool getNumber(const std::string& key, double& value) const;

    double getNumber(const std::string& key) const;

    bool getString(const std::string& key, std::string& value) const;

    const std::string& getString(const std::string& key) const;

    std::string asString(const Value& value) const;

    std::string getAsString(const std::string& key) const;

    bool getAsString(const std::string& key, std::string& value) const;

    std::string toJson() const;

    void set(std::string key, std::string value);
    void set(std::string key, double value);

    void setSorted(std::vector<Item>&& _items);

    // template <typename... Args> void set(std::string key, Args&&... args) {
    //     props.emplace_back(std::move(key), Value{std::forward<Args>(args)...});
    //     sort();
    // }

    const std::vector<Item>& items() const { return props; }

    int32_t sourceId;

    static bool keyComparator(const std::string& a, const std::string& b) {
        if (a.size() == b.size()) {
            return a < b;
        } else {
            return a.size() < b.size();
        }
    }
private:
    std::vector<Item> props;
};

}
