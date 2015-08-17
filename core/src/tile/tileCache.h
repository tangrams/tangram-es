#pragma once

#include "tileID.h"
#include "tile.h"

#include <unordered_map>
#include <list>
#include <memory>

namespace Tangram {
    using TileCacheKey = std::pair<DataSource*, TileID>;
}

namespace std {
    template <>
    struct hash<Tangram::TileCacheKey> {
        size_t operator()(const Tangram::TileCacheKey& k) const {
            std::size_t seed = 0;
            hash_combine(seed, k.first);
            hash_combine(seed, k.second);
            return seed;
        }
    };
}

namespace Tangram {

class TileCache {
    using CacheList = std::list<std::pair<TileCacheKey, std::shared_ptr<Tile>>>;
    using CacheMap = std::unordered_map<TileCacheKey, typename CacheList::iterator>;

public:
    void put(DataSource& _source, std::shared_ptr<Tile> _tile) {
        TileCacheKey k(&_source, _tile->getID());

        m_cacheList.push_front({k, _tile});
        m_cacheMap[k] = m_cacheList.begin();

        if ( m_cacheList.size() > m_maxEntries ) {
            m_cacheMap.erase(m_cacheList.back().first);
            m_cacheList.pop_back();
        }
    }

    std::shared_ptr<Tile> get(DataSource& _source, TileID _tileID) {
        std::shared_ptr<Tile> tile;
        TileCacheKey k(&_source, _tileID);

        auto it = m_cacheMap.find(k);
        if (it != m_cacheMap.end()) {
            std::swap(tile, (*(it->second)).second);
            m_cacheList.erase(it->second);
            m_cacheMap.erase(it);
        }
        return tile;
    }

    size_t size() const { return m_cacheList.size(); }

    size_t getMemoryUsage() const {
        size_t sum = 0;

        for (auto& entry : m_cacheList) {
            sum += entry.second->getMemoryUsage();
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
