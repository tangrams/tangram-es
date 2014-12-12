#pragma once

#include <vector>

/* An immutable identifier for a map tile 
 * 
 * Contains the x, y, and z indices of a tile in a quad tree; TileIDs are arbitrarily but strictly ordered
 */

struct TileID {

    TileID(int _x, int _y, int _z) : x(_x), y(_y), z(_z) {};


    bool operator< (const TileID& _rhs) const {
        if(x != _rhs.x) {
            return (x < _rhs.x);
        }
        else if(y != _rhs.y) {
            return (y < _rhs.y);
        }
        else {
            return (z < _rhs.z);
        }
    }
    bool operator> (const TileID& _rhs) const { return _rhs < const_cast<TileID&>(*this); }
    bool operator<=(const TileID& _rhs) const { return !(*this > _rhs); }
    bool operator>=(const TileID& _rhs) const { return !(*this < _rhs); }
    bool operator==(const TileID& _rhs) const { return x == _rhs.x && y == _rhs.y && z == _rhs.z; }

    const int x;
    const int y;
    const int z;

    TileID* getParent() const {
        TileID* _parent = nullptr;
        if( (x >> 1) >= 0 && (y >> 1) >= 0 && (z-1) >= 0) {
            _parent = new TileID(x >> 1, y >> 1, z-1);
        }
        return _parent;
    }

    void getChildren(std::vector<TileID>& _children, int _maxZoomLvl) const {
        if((z+1) <= _maxZoomLvl) {
            for(int i = 0; i < 2; i++) {
                int xVal = (x << 1) + i;
                for(int j = 0; j < 2; j++) {
                    int yVal = (y << 1) + j;
                    _children.push_back(TileID(xVal, yVal, z+1));
                }
            }
        }
    }
};

