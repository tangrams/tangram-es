#pragma once

/* An immutable identifier for a map tile 
 * 
 * Contains the x, y, and z indices of a tile in a quad tree; TileIDs are arbitrarily but strictly ordered
 */

struct TileID {

    TileID(int _x, int _y, int _z) : x(_x), y(_y), z(_z) {};


    bool operator< (const TileID& _rhs) const { return (x < _rhs.x || (y < _rhs.y || z < _rhs.z)); }
    bool operator> (const TileID& _rhs) const { return _rhs < const_cast<TileID&>(*this); }
    bool operator<=(const TileID& _rhs) const { return !(*this > _rhs); }
    bool operator>=(const TileID& _rhs) const { return !(*this < _rhs); }
    bool operator==(const TileID& _rhs) const { return x == _rhs.x && y == _rhs.y && z == _rhs.z; }

    const int x;
    const int y;
    const int z;
};

