#pragma once

#include "tile/tileID.h"
#include "util/hash.h"

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
