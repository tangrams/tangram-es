#pragma once

#include <cassert>
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

    TileID(int32_t _x, int32_t _y, int32_t _z, int32_t _s) : x(_x), y(_y), z(_z), s(_s) {}

    TileID(int32_t _x, int32_t _y, int32_t _z) : TileID(_x, _y, _z, _z) {}

    TileID(const TileID& _rhs) = default;

    bool operator< (const TileID& _rhs) const {
        return s > _rhs.s || (s == _rhs.s && (z > _rhs.z || (z == _rhs.z && (x < _rhs.x || (x == _rhs.x && (y < _rhs.y))))));
    }
    bool operator> (const TileID& _rhs) const { return _rhs < const_cast<TileID&>(*this); }
    bool operator<=(const TileID& _rhs) const { return !(*this > _rhs); }
    bool operator>=(const TileID& _rhs) const { return !(*this < _rhs); }
    bool operator==(const TileID& _rhs) const { return x == _rhs.x && y == _rhs.y && z == _rhs.z && s == _rhs.s; }
    bool operator!=(const TileID& _rhs) const { return !(*this == _rhs); }

    bool isValid() const {
        int max = 1 << z;
        return x >= 0 && x < max && y >= 0 && y < max && z >= 0;
    }

    bool isValid(int _maxZoom) const {
        return isValid() && z <= _maxZoom;
    }

    TileID withMaxSourceZoom(int32_t _maxZoom) const {

        if (z <= _maxZoom) {
            return *this;
        }

        int32_t over = z - _maxZoom;

        return TileID(x >> over, y >> over, _maxZoom, s);
    }

    TileID zoomBiasAdjusted(int32_t _zoomBias) const {
        assert(_zoomBias >= 0);

        if (!_zoomBias) { return *this; }

        auto scaledZ = std::max(0, z - _zoomBias);
        return TileID(x >> _zoomBias, y >> _zoomBias, scaledZ, z);
    }

    TileID getParent(int32_t _zoomBias = 0) const {
        if (s > (z + _zoomBias)) {
            // Over-zoomed, keep the same data coordinates
            return TileID(x, y, z, s - 1);
        }
        return TileID(x >> 1, y >> 1, z - 1, s - 1);
    }

    TileID getChild(int32_t _index, int32_t _maxZoom) const {
        if (_index > 3 || _index < 0) {
            return TileID(-1, -1, -1, -1);
        }

        int i = _index / 2;
        int j = _index % 2;

        // _index: 0, 1, 2, 3
        // i:      0, 0, 1, 1
        // j:      0, 1, 0, 1

        auto childID = TileID((x<<1)+i, (y<<1)+j, z + 1, s + 1);
        return childID.withMaxSourceZoom(_maxZoom);
    }

    std::string toString() const {
        return std::to_string(x) + "/" + std::to_string(y) + "/" + std::to_string(z) + "/s:" + std::to_string(s);
    }

};

static const TileID NOT_A_TILE(-1, -1, -1, -1);

}
