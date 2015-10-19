#pragma once

#include <string>

/* An immutable identifier for a map tile
 *
 * Contains the x, y, and z indices of a tile in a quad tree; TileIDs are ordered by:
 * 1. z, highest to lowest
 * 2. x, lowest to highest
 * 3. y, lowest to highest
 */

namespace Tangram {

enum class TileWrap {
    none, // no tile wrapping
    negative, //negative wrap
    positive, //positive wrap
};

struct TileID {

    const int x;
    const int y;
    const int z;

    TileWrap wrap = TileWrap::none;

    TileID(int _x, int _y, int _z, TileWrap _wrap = TileWrap::none) : x(_x), y(_y), z(_z), wrap(_wrap) {};
    TileID(const TileID& _rhs): x(_rhs.x), y(_rhs.y), z(_rhs.z), wrap(_rhs.wrap) {};

    bool operator< (const TileID& _rhs) const {
        return z > _rhs.z || (z == _rhs.z && (x < _rhs.x || (x == _rhs.x && (y < _rhs.y || (y == _rhs.y && wrap < _rhs.wrap)))));
    }
    bool operator> (const TileID& _rhs) const { return _rhs < const_cast<TileID&>(*this); }
    bool operator<=(const TileID& _rhs) const { return !(*this > _rhs); }
    bool operator>=(const TileID& _rhs) const { return !(*this < _rhs); }
    bool operator==(const TileID& _rhs) const { return x == _rhs.x && y == _rhs.y && z == _rhs.z && wrap == _rhs.wrap; }

    TileID& operator=(const TileID& _rhs) = delete;

    bool isValid() const {
        int max = 1 << z;
        return x >= 0 && x < max && y >= 0 && y < max && z >= 0;
    }

    bool isValid(int _maxZoom) const {
        return isValid() && z <= _maxZoom;
    }

    TileID getParent() const {
        return TileID(x >> 1, y >> 1, z-1, wrap);
    }

    TileID getChild(int _index) const {

        if (_index > 3 || _index < 0) {
            return TileID(-1, -1, -1);
        }

        int i = _index / 2;
        int j = _index % 2;

        // _index: 0, 1, 2, 3
        // i:      0, 0, 1, 1
        // j:      0, 1, 0, 1

        return TileID((x<<1)+i, (y<<1)+j, z+1, wrap);
    }

    std::string toString() const {
        return std::to_string(x) + "/" + std::to_string(y) + "/" + std::to_string(z);
    }

};

static TileID NOT_A_TILE(-1, -1, -1);

}
