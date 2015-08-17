#pragma once

#include "tileID.h"
#include "tile.h"

#include <unordered_map>
#include <list>
#include <memory>

namespace Tangram {

class TileCache {
    using CacheList = std::list<std::shared_ptr<Tile>>;
    using CacheMap = std::unordered_map<TileID, typename CacheList::iterator>;

    // 32 MB
    const static size_t DEFAULT_CACHE_SIZE = 32*1024*1024;

public:

    TileCache(size_t _cacheSizeMB = DEFAULT_CACHE_SIZE) : m_cacheMaxUsage(_cacheSizeMB) {}

    void put(std::shared_ptr<Tile> _tile) {
        m_cacheList.push_front(_tile);
        m_cacheMap[_tile->getID()] = m_cacheList.begin();
        m_cacheUsage += _tile->getMemoryUsage();

        while (m_cacheUsage > m_cacheMaxUsage) {
            auto& tile = m_cacheList.back();
            m_cacheUsage -= tile->getMemoryUsage();
            m_cacheMap.erase(tile->getID());
            m_cacheList.pop_back();
        }
    }

    std::shared_ptr<Tile> get(TileID _tileID) {
        std::shared_ptr<Tile> tile;
        auto it = m_cacheMap.find(_tileID);
        if (it != m_cacheMap.end()) {
            std::swap(tile, *(it->second));
            m_cacheList.erase(it->second);
            m_cacheMap.erase(it);

            m_cacheUsage -= tile->getMemoryUsage();
        }
        return tile;
    }

    size_t size() const { return m_cacheList.size(); }

    size_t getMemoryUsage() const {
        size_t sum = 0;

        for (auto& tile : m_cacheList) {
            sum += tile->getMemoryUsage();
        }
        return sum;
    }

    void clear() {
        m_cacheMap.clear();
        m_cacheList.clear();
    }

private:
    CacheMap m_cacheMap;
    CacheList m_cacheList;

    size_t m_cacheUsage;
    size_t m_cacheMaxUsage;
};

}
