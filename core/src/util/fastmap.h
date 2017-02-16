#pragma once

#include <algorithm>
#include <string>
#include <vector>

namespace Tangram {

// see also:
// http://www.boost.org/doc/libs/1_59_0/doc/html/boost/container/flat_map.html
// https://realm.io/assets/news/binary-search/blog.cpp

template<typename K, typename T>
struct fastmap {
    std::vector<std::pair<K, T>> map;

    using iterator = typename std::vector<std::pair<K, T>>::iterator;
    using const_iterator = typename std::vector<std::pair<K, T>>::const_iterator;

    T& operator[](const K& key) {
        iterator it = std::lower_bound(
            map.begin(), map.end(), key,
            [&](const auto& item, const auto& key) {
                return item.first < key;
            });

        if (it == map.end() || it->first != key) {
            auto entry = map.emplace(it, key, T{});
            return entry->second;
        }

        return it->second;
    }

    const_iterator find(const K& key) const {
        const_iterator it = std::lower_bound(
            map.begin(), map.end(), key,
            [&](const auto& item, const auto& key) {
                return item.first < key;
            });

        if (it == map.end() || it->first == key) {
            return it;
        }
        return map.end();
    }

    iterator find(const K& key) {
        iterator it = std::lower_bound(
            map.begin(), map.end(), key,
            [&](const auto& item, const auto& key) {
                return item.first < key;
            });

        if (it == map.end() || it->first == key) {
            return it;
        }
        return map.end();
    }

    iterator erase(const_iterator position) {
        return map.erase(position);
    }

    iterator begin() { return map.begin(); }
    iterator end() { return map.end(); }

    const_iterator begin() const { return map.begin(); }
    const_iterator end() const { return map.end(); }

    size_t size() const { return map.size(); }

    void clear() { map.clear(); }
};

template<typename T>
struct fastmap<std::string, T> {

    struct Key {
        size_t hash;
        std::string k;
    };

    using iterator = typename std::vector<std::pair<Key, T>>::iterator;
    using const_iterator = typename std::vector<std::pair<Key, T>>::const_iterator;

    std::vector<std::pair<Key, T>> map;
    std::vector<size_t> lengths;

    T& operator[](const std::string& key) {

        size_t hash = std::hash<std::string>()(key);
        iterator it = std::lower_bound(
            map.begin(), map.end(), key,
            [&](const auto& item, const auto& key) {
                if (item.first.hash == hash) {
                    return item.first.k < key;
                }
                return item.first.hash < hash;
            });

        if (it == map.end() || it->first.k != key) {
            auto entry = map.emplace(it, Key{hash, key}, T{});
            return entry->second;
        }

        return it->second;
    }

    const_iterator find(const std::string& key) const {
        size_t hash = std::hash<std::string>()(key);

        const_iterator it = std::lower_bound(
            map.begin(), map.end(), key,
            [&](const auto& item, const auto& key) {
                if (item.first.hash == hash) {
                    return item.first.k < key;
                }
                return item.first.hash < hash;
            });

        if (it == map.end() || it->first.k == key) {
            return it;
        }
        return map.end();
    }
    void clear() { map.clear(); }

    iterator begin() { return map.begin(); }
    iterator end() { return map.end(); }

    size_t size() const { return map.size(); }

    const_iterator begin() const { return map.begin(); }
    const_iterator end() const { return map.end(); }
};

}
