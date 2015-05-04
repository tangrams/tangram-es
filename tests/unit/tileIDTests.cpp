#define CATCH_CONFIG_MAIN 
#include "catch.hpp"

#include "util/tileID.h"
#include <set>

TEST_CASE( "Create TileIDs and check that they are correctly ordered", "[Core][TileID]" ) {
    
    TileID a = TileID(1, 1, 1);

    REQUIRE(a.x == 1);
    REQUIRE(a.y == 1);
    REQUIRE(a.z == 1);

    TileID b = TileID(2, 1, 1);
    TileID c = TileID(1, 2, 1);
    TileID d = TileID(1, 1, 2);

    REQUIRE(a < b);
    REQUIRE(b > a);

    REQUIRE(a < c);
    REQUIRE(c > a);

    REQUIRE(a > d);
    REQUIRE(d < a);

    REQUIRE(c < b);
    REQUIRE(b > c);

    REQUIRE(b > d);
    REQUIRE(d < b);

    REQUIRE(c > d);
    REQUIRE(d < c);
    
}

TEST_CASE( "Create TileIDs and find their parents and children", "[Core][TileID]" ) {

    TileID a = TileID(1, 2, 3);

    TileID parent = a.getParent();

    REQUIRE(parent == TileID(0, 1, 2));

    std::set<TileID> children;

    for (int i = 0; i < 4; i++) {
        children.insert(a.getChild(i));
    }

    std::set<TileID> requiredChildren = { TileID(2, 4, 4), TileID(3, 4, 4), TileID(2, 5, 4), TileID(3, 5, 4) };

    REQUIRE(children == requiredChildren);

    for (const TileID& t : requiredChildren) {
        REQUIRE(t.getParent() == a);
    }

}

TEST_CASE( "Ensure TileIDs correctly evaluate their validity", "[Core][TileID]") {

    REQUIRE(TileID(1, 1, 1).isValid());
    REQUIRE(TileID(1, 1, 1).isValid(1));
    REQUIRE(!TileID(1, 1, 2).isValid(1));

    REQUIRE(!TileID(-1,  0,  0).isValid());
    REQUIRE(!TileID( 0, -1,  0).isValid());
    REQUIRE(!TileID( 0,  0, -1).isValid());
    REQUIRE(!TileID( 1,  0,  0).isValid());
    REQUIRE(!TileID( 0,  1,  0).isValid());

    REQUIRE(!NOT_A_TILE.isValid());
    
}
