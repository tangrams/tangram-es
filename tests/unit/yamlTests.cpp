#include "catch.hpp"

#include <iostream>
#include <vector>

#include "yaml-cpp/yaml.h"


TEST_CASE( "yaml copy constructor", "[yaml]") {
    std::vector<YAML::Node> nodes;

    for (int i = 0; i < 100; i++) {
        auto it = nodes.emplace(nodes.begin(), std::to_string(i));
        printf(">>> %d - %s\n", i, it->Scalar().c_str());
    }

    for (int i = 0; i < 100; i++) {
        printf("<<< %d - %s\n", i, nodes[99 - i].Scalar().c_str());
    }
    for (int i = 0; i < 100; i++) {
        REQUIRE(nodes[99 - i].Scalar() == std::to_string(i));
    }
}
