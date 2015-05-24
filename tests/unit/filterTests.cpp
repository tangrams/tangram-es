#define CATCH_CONFIG_MAIN 
#include "catch.hpp"

#include "filters.h"
#include "stl_util.hpp"

TEST_CASE( "filtersssss", "[filters][core]" ) {

    using namespace Tangram;

    Feature feat;
    feat.geometryType = GeometryType::LINES;
    feat.props.numericProps["legs"] = 8;
    feat.props.numericProps["spines"] = 0;
    feat.props.stringProps["kingdom"] = "animalia";
    feat.props.stringProps["phylum"] = "mollusca";
    feat.props.stringProps["class"] = "cephalopoda";
    feat.props.stringProps["order"] = "octopoda";

    Context ctx;

    auto* isAlive = new Equality { "kingdom", { new StrValue("animalia"), new StrValue("plantae") } };

    REQUIRE(isAlive->eval(feat, ctx));

    auto* hasEightLegs = new Equality { "legs", { new NumValue(8) } };

    auto* hasEightLegsAndAlive = new All { { hasEightLegs, isAlive } };

    REQUIRE(hasEightLegsAndAlive->eval(feat, ctx));

    delete hasEightLegsAndAlive;

}