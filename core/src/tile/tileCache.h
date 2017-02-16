#pragma once

#include "log.h"
#include "tile/tile.h"
#include "tile/tileHash.h"
#include "tile/tileID.h"

#include <list>
#include <memory>
#include <unordered_map>

namespace Tangram {
// TileSet serial + TileID
using TileCacheKey = std::pair<int32_t, TileID>;
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
    struct CacheEntry {
        TileCacheKey key;
        std::shared_ptr<Tile> tile;
    };

    using CacheList = std::list<CacheEntry>;
    using CacheMap = std::unordered_map<TileCacheKey, typename CacheList::iterator>;

public:

    TileCache(size_t _cacheSizeMB) :
        m_cacheUsage(0),
        m_cacheMaxUsage(_cacheSizeMB) {}

    std::vector<TileID> put(int32_t _sourceId, std::shared_ptr<Tile> _tile) {
        TileCacheKey k(_sourceId, _tile->getID());

        m_cacheList.push_front({k, _tile});
        m_cacheMap[k] = m_cacheList.begin();
        m_cacheUsage += _tile->getMemoryUsage();

        return limitCacheSize(m_cacheMaxUsage);
    }

    std::shared_ptr<Tile> get(int32_t _sourceId, TileID _tileId) {
        std::shared_ptr<Tile> tile;
        TileCacheKey k(_sourceId, _tileId);

        auto it = m_cacheMap.find(k);
        if (it != m_cacheMap.end()) {
            std::swap(tile, (*(it->second)).tile);
            m_cacheList.erase(it->second);
            m_cacheMap.erase(it);
            m_cacheUsage -= tile->getMemoryUsage();
        }
        return tile;
    }

    std::shared_ptr<Tile> contains(int32_t _source, TileID _tileID) {
        std::shared_ptr<Tile> tile;
        TileCacheKey k(_source, _tileID);

        auto it = m_cacheMap.find(k);
        if (it != m_cacheMap.end()) {
            return it->second->tile;
        }
        return nullptr;
    }

    std::vector<TileID> limitCacheSize(size_t _cacheSizeBytes) {
        std::vector<TileID> poppedTileIDs;
        m_cacheMaxUsage = _cacheSizeBytes;

        while (m_cacheUsage > m_cacheMaxUsage) {
            if (m_cacheList.empty()) {
                LOGE("Invalid cache state!");
                m_cacheUsage = 0;
                break;
            }
            auto& tile = m_cacheList.back().tile;
            poppedTileIDs.push_back(tile->getID());
            m_cacheUsage -= tile->getMemoryUsage();
            m_cacheMap.erase(m_cacheList.back().key);
            m_cacheList.pop_back();
        }
        return poppedTileIDs;
    }

    size_t getMemoryUsage() const {
        size_t sum = 0;
        for (auto& entry : m_cacheList) {
            sum += entry.tile->getMemoryUsage();
        }
        return sum;
    }

    void clear() {
        m_cacheMap.clear();
        m_cacheList.clear();
        m_cacheUsage = 0;
    }

private:
    CacheMap m_cacheMap;
    CacheList m_cacheList;

    int m_cacheUsage;
    int m_cacheMaxUsage;
};

}
