#include "catch.hpp"

#include <iostream>
#include <vector>
#include <string>

#include "yaml-cpp/yaml.h"
#include "scene/filters.h"
#include "scene/sceneLoader.h"
#include "style/polygonStyle.h"

using namespace Tangram;
using YAML::Node;

TEST_CASE( "Style Sorting Test", "[styleSorting][core][yaml]") {

    std::vector<std::unique_ptr<Style>> styles;

    std::unique_ptr<PolygonStyle> s1(new PolygonStyle("s-none-none"));
    std::unique_ptr<PolygonStyle> s2(new PolygonStyle("t-overlay-3"));
    std::unique_ptr<PolygonStyle> s3(new PolygonStyle("a-add-10"));
    std::unique_ptr<PolygonStyle> s4(new PolygonStyle("r-none-none"));
    std::unique_ptr<PolygonStyle> s5(new PolygonStyle("w-inlay-0"));
    std::unique_ptr<PolygonStyle> s6(new PolygonStyle("a2-overlay-3"));
    std::unique_ptr<PolygonStyle> s7(new PolygonStyle("r2-none-4"));
    std::unique_ptr<PolygonStyle> s8(new PolygonStyle("s2-multiply-10"));

    s2->setBlendMode(Blending::overlay);
    s3->setBlendMode(Blending::add);
    s5->setBlendMode(Blending::inlay);
    s8->setBlendMode(Blending::multiply);
    s6->setBlendMode(Blending::overlay);

    s7->setBlendOrder(4); //Test opaque style with a blend order (should not matter)
    s2->setBlendOrder(3);
    s3->setBlendOrder(10);
    s5->setBlendOrder(0);
    s8->setBlendOrder(10);
    s6->setBlendOrder(3);

    styles.push_back(std::move(s1));
    styles.push_back(std::move(s2));
    styles.push_back(std::move(s3));
    styles.push_back(std::move(s4));
    styles.push_back(std::move(s5));
    styles.push_back(std::move(s6));
    styles.push_back(std::move(s7));
    styles.push_back(std::move(s8));

    std::sort(styles.begin(), styles.end(), Style::compare);

    REQUIRE(styles[0]->getName() == "r-none-none");
    REQUIRE(styles[1]->getName() == "r2-none-4");
    REQUIRE(styles[2]->getName() == "s-none-none");
    REQUIRE(styles[3]->getName() == "w-inlay-0");
    REQUIRE(styles[4]->getName() == "a2-overlay-3");
    REQUIRE(styles[5]->getName() == "t-overlay-3");
    REQUIRE(styles[6]->getName() == "a-add-10");
    REQUIRE(styles[7]->getName() == "s2-multiply-10");
}
