#pragma once

#include "tileID.h"

#include <functional> // for hash function

// The generic hash_combine used in Boost
// http://www.boost.org/doc/libs/1_35_0/doc/html/boost/hash_combine_id241013.html
template <class T>
inline void hash_combine(std::size_t & seed, const T & v) {
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

namespace std {
    template <>
    struct hash<Tangram::TileID> {
        size_t operator()(const Tangram::TileID& k) const {
            std::size_t seed = 0;
            hash_combine(seed, k.x);
            hash_combine(seed, k.y);
            hash_combine(seed, k.z);
            return seed;
        }
    };
}
