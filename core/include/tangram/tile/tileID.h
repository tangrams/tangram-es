#pragma once

#include <cstdint>
#include <string>

/* An immutable identifier for a map tile
 *
 * Contains the x, y, and z indices of a tile in a quad tree; TileIDs are ordered by:
 * 1. z, highest to lowest
 * 2. x, lowest to highest
 * 3. y, lowest to highest
 */

namespace Tangram {

struct TileID {

    int32_t x; // Index from left edge of projection space
    int32_t y; // Index from top edge of projection space
    int8_t  z; // Data zoom
    int8_t  s; // Styling zoom
    int16_t wrap;

    TileID(int32_t _x, int32_t _y, int32_t _z, int32_t _s, int32_t _wrap) : x(_x), y(_y), z(_z), s(_s), wrap(_wrap) {}

    TileID(int32_t _x, int32_t _y, int32_t _z) : TileID(_x, _y, _z, _z, 0) {}

    TileID(const TileID& _rhs) = default;

    bool operator< (const TileID& _rhs) const {
        return s > _rhs.s || (s == _rhs.s && (z > _rhs.z || (z == _rhs.z && (x < _rhs.x || (x == _rhs.x && (y < _rhs.y || (y == _rhs.y && wrap < _rhs.wrap)))))));
    }
    bool operator> (const TileID& _rhs) const { return _rhs < const_cast<TileID&>(*this); }
    bool operator<=(const TileID& _rhs) const { return !(*this > _rhs); }
    bool operator>=(const TileID& _rhs) const { return !(*this < _rhs); }
    bool operator==(const TileID& _rhs) const { return x == _rhs.x && y == _rhs.y && z == _rhs.z && s == _rhs.s && wrap == _rhs.wrap; }
    bool operator!=(const TileID& _rhs) const { return !(*this == _rhs); }

    bool isValid() const {
        int max = 1 << z;
        return x >= 0 && x < max && y >= 0 && y < max && z >= 0;
    }

    bool isValid(int _maxZoom) const {
        return isValid() && z <= _maxZoom;
    }

    TileID withMaxSourceZoom(int32_t _max) const {

        if (z <= _max) {
            return *this;
        }

        int32_t over = z - _max;

        return TileID(x >> over, y >> over, _max, z, wrap);
    }

    TileID getParent() const {

        if (s > z) {
            // Over-zoomed, keep the same data coordinates
            return TileID(x, y, z, s - 1, wrap);
        }
        return TileID(x >> 1, y >> 1, z-1, z-1, wrap);
    }

    TileID getChild(int32_t _index) const {

        if (_index > 3 || _index < 0) {
            return TileID(-1, -1, -1, -1, -1);
        }

        int i = _index / 2;
        int j = _index % 2;

        // _index: 0, 1, 2, 3
        // i:      0, 0, 1, 1
        // j:      0, 1, 0, 1

        return TileID((x<<1)+i, (y<<1)+j, z+1, z+1, wrap);
    }

    TileID getChild(int32_t _index, int32_t _maxSourceZoom) const {
        return getChild(_index).withMaxSourceZoom(_maxSourceZoom);
    }

    std::string toString() const {
        return std::to_string(x) + "/" + std::to_string(y) + "/" + std::to_string(z);
    }

};

static const TileID NOT_A_TILE(-1, -1, -1, -1, -1);

}
