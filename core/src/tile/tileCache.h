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

public:
    void put(std::shared_ptr<Tile> _tile) {
        m_cacheList.push_front(_tile);
        m_cacheMap[_tile->getID()] = m_cacheList.begin();

        if ( m_cacheList.size() > m_maxEntries ) {
            m_cacheMap.erase(m_cacheList.back()->getID());
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

    size_t m_maxEntries = 40;
};

}
